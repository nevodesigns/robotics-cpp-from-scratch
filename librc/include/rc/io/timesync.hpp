// rc/io/timesync.hpp
//
// The device clock handling from lesson 08-12, graduated.
//
// A device stamps its messages with its own clock, and its clock is wrong in three
// ways. The counter wraps, so TickUnwrapper turns it into a running total, taking the
// difference in the counter's own width: done in int, two stamps across a sixteen bit
// wrap are minus 65526 ms apart. The clock starts at an arbitrary point. And it runs a
// little fast or slow, so the offset is not constant.
//
// ClockMap fits a line through the least delayed sample of each ten seconds. Measured
// over an hour with a 50 ppm fast device and a link delaying by two to ten
// milliseconds, the arrival time was out by 8.0 ms, the first offset used for ever by
// 185.3 ms, and the map by 0.84 ms.
//
// A restart is a step in the device's clock. A map that is not reset when the
// unwrapper reports one was wrong by the whole downtime, 5011 ms for a five second
// reset, for ten minutes.

#ifndef RC_IO_TIMESYNC_HPP
#define RC_IO_TIMESYNC_HPP

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <vector>

namespace rc {
namespace io {

// What a device's timestamp turned into.
struct Unwrapped {
  std::uint64_t ticks = 0;    // device ticks since the first message, never going backwards
  bool restarted = false;     // the device's clock went backwards: it has been reset
};

// Turns a counter that wraps into one that does not.
//
// A device stamps each message with its own tick counter, and a counter has a
// width. Sixteen bits of milliseconds wraps every 65.5 seconds, and a robot that
// subtracts two stamps across the wrap gets a duration of minus a minute.
//
// The difference has to be taken in the counter's own width, so that it wraps the
// way the counter does. A serial link delivers in order, so a stamp that went
// backwards is not a message that arrived late: the device was reset.
template <class T>
class TickUnwrapper {
  static_assert(std::is_unsigned<T>::value, "a tick counter is unsigned, so that it wraps");

 public:
  Unwrapped accept(T raw) {
    Unwrapped out;

    if (!started_) {
      started_ = true;
      last_ = raw;
      total_ = 0;
      out.ticks = total_;
      return out;
    }

    // In T. Written as raw - last_ it is done in int after promotion, and a
    // wrap becomes a large negative number.
    const T forward = static_cast<T>(raw - last_);

    if (forward < kHalf) {
      total_ += forward;
    } else {
      // More than half a turn ahead is the same as a little behind. A link
      // delivers in order, so that is the device starting again, and the
      // timeline carries on from where it was.
      out.restarted = true;
      ++restarts_;
    }

    last_ = raw;
    out.ticks = total_;
    return out;
  }

  std::size_t restarts() const { return restarts_; }

 private:
  static constexpr T kHalf = static_cast<T>(static_cast<T>(~T(0)) / 2u + 1u);

  bool started_ = false;
  T last_ = 0;
  std::uint64_t total_ = 0;
  std::size_t restarts_ = 0;
};

// Maps the device's clock to the host's.
//
// Three things are wrong with a device's timestamp, and each needs a different
// remedy. It wraps, which the unwrapper handles. It starts at an arbitrary point,
// so there is an offset. And its oscillator runs a little fast or slow, so the
// offset is not constant: a crystal is good for tens of parts per million and a
// cheaper oscillator for thousands.
//
// What the host can see is the time each message arrived, which is the time it
// was sent plus a delay that is never negative. So the best estimate of the
// device-to-host relation is a line that lies under the arrival times and touches
// the ones that were least delayed. This keeps the least delayed sample in each
// stretch of device time, a bucket, and fits a line through those.
//
// What it cannot know is the smallest delay itself. The line lies at the send time
// plus that delay, which is a constant error and cannot be removed without a
// second clock.
class ClockMap {
 public:
  explicit ClockMap(double bucket_seconds = 10.0, std::size_t buckets = 60)
      : bucket_seconds_(bucket_seconds > 0.0 ? bucket_seconds : 10.0),
        slots_(buckets < 2 ? 2 : buckets) {}

  // device is the unwrapped device time in seconds, host is when it arrived.
  void add(double device, double host) {
    const long long index = static_cast<long long>(std::floor(device / bucket_seconds_));
    const std::size_t at = static_cast<std::size_t>(((index % count()) + count()) % count());
    Slot& slot = slots_[at];

    if (!slot.used || slot.index != index) {
      slot.used = true;
      slot.index = index;
      slot.device = device;
      slot.host = host;
    } else if (host - device < slot.host - slot.device) {
      slot.device = device;     // a less delayed sample in the same bucket
      slot.host = host;
    }

    if (!any_ || index > newest_) newest_ = index;
    any_ = true;
  }

  void reset() {
    for (Slot& slot : slots_) slot = Slot{};
    any_ = false;
    newest_ = 0;
  }

  // Two buckets are what it takes to see a slope.
  bool ready() const { return fit().points >= 2; }

  // The host time that goes with a device time. Before there is anything to fit
  // it is the device time unchanged, and with one bucket it is an offset only.
  double to_host(double device) const {
    const Fit f = fit();
    if (f.points == 0) return device;
    return f.slope * (device - f.mean_device) + f.mean_host;
  }

  // How much faster the device's clock runs than the host's, in parts per million.
  // The slope is host seconds per device second, so a fast device has a slope a
  // little under one and this comes out positive.
  double drift_ppm() const { return (1.0 / fit().slope - 1.0) * 1e6; }

 private:
  struct Slot {
    bool used = false;
    long long index = 0;
    double device = 0.0;
    double host = 0.0;
  };

  struct Fit {
    std::size_t points = 0;
    double slope = 1.0;
    double mean_device = 0.0;
    double mean_host = 0.0;
  };

  long long count() const { return static_cast<long long>(slots_.size()); }

  // A least squares line through the least delayed sample of each bucket still
  // in the window, taken about the means, which is the well conditioned way to
  // do it.
  Fit fit() const {
    Fit f;
    if (!any_) return f;

    for (const Slot& slot : slots_) {
      if (!slot.used || slot.index <= newest_ - count()) continue;
      ++f.points;
      f.mean_device += slot.device;
      f.mean_host += slot.host;
    }
    if (f.points == 0) return f;
    f.mean_device /= static_cast<double>(f.points);
    f.mean_host /= static_cast<double>(f.points);

    double spread = 0.0;
    double cross = 0.0;
    for (const Slot& slot : slots_) {
      if (!slot.used || slot.index <= newest_ - count()) continue;
      const double dx = slot.device - f.mean_device;
      spread += dx * dx;
      cross += dx * (slot.host - f.mean_host);
    }
    if (f.points >= 2 && spread > 0.0) f.slope = cross / spread;
    return f;
  }

  double bucket_seconds_;
  std::vector<Slot> slots_;
  bool any_ = false;
  long long newest_ = 0;
};

}  // namespace io
}  // namespace rc

#endif  // RC_IO_TIMESYNC_HPP
