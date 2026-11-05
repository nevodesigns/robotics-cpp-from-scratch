#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <vector>

// What a device's timestamp turned into.
struct Unwrapped {
  std::uint64_t ticks = 0;    // device ticks since the first message, never going backwards
  bool restarted = false;     // the device's clock went backwards: it has been reset
};

// Turns a counter that wraps into one that does not.
template <class T>
class TickUnwrapper {
  static_assert(std::is_unsigned<T>::value, "a tick counter is unsigned, so that it wraps");

 public:
  Unwrapped accept(T raw) {
    // TODO: the first stamp starts the timeline at zero ticks.
    //
    // After that, how far ahead of the last stamp this one is has to be computed
    // IN T. Written as raw - last_ it is done in int after promotion, and across
    // the wrap it is minus 65526 instead of plus ten. static_cast it back to T.
    //
    // Less than half the range ahead: add it to total_. At least half: the clock
    // went backwards. A serial link delivers in order, so that is the device being
    // reset and not a message that arrived late. Set restarted, count it in
    // restarts_, leave total_ alone so the timeline carries on, and take raw as
    // the new last stamp either way.
    (void)raw;
    (void)started_;
    (void)last_;
    (void)total_;
    return Unwrapped{};
  }

  std::size_t restarts() const { return restarts_; }

 private:
  bool started_ = false;
  T last_ = 0;
  std::uint64_t total_ = 0;
  std::size_t restarts_ = 0;
};

// Maps the device's clock to the host's.
//
// What the host can see is when each message arrived, which is when it was sent
// plus a delay that is never negative. So the best estimate of the relation is a
// line under the arrival times that touches the least delayed ones. Keep the least
// delayed sample in each stretch of device time, a bucket, and fit a line through
// those.
class ClockMap {
 public:
  explicit ClockMap(double bucket_seconds = 10.0, std::size_t buckets = 60)
      : bucket_seconds_(bucket_seconds > 0.0 ? bucket_seconds : 10.0),
        slots_(buckets < 2 ? 2 : buckets) {}

  // device is the unwrapped device time in seconds, host is when it arrived.
  void add(double device, double host) {
    // TODO: find the bucket this device time falls in, floor(device / bucket),
    // and the slot for it, which is the bucket number modulo the number of slots
    // (a plain % can be negative for a negative number, so make it positive).
    //
    // A slot that is empty, or that still holds an older bucket, is replaced by
    // this sample. A slot already holding this bucket keeps whichever sample was
    // LEAST delayed, which is the one with the smaller host - device.
    //
    // Remember the newest bucket seen, so that slots left over from a full turn
    // ago can be told apart from the current window.
    (void)device;
    (void)host;
    (void)bucket_seconds_;
    (void)slots_;
    (void)newest_;
  }

  void reset() {
    // TODO: forget every sample.
    (void)any_;
  }

  // Two buckets are what it takes to see a slope.
  bool ready() const {
    // TODO
    return false;
  }

  // The host time that goes with a device time. Before there is anything to fit it
  // is the device time unchanged, and with one bucket it is an offset only.
  double to_host(double device) const {
    // TODO: a least squares line through the least delayed sample of each bucket
    // still in the window, that is bucket index greater than newest_ minus the
    // number of slots. Work about the means: slope * (device - mean_device) +
    // mean_host. One point gives slope one. No points gives the device time.
    return device;
  }

  // How much faster the device's clock runs than the host's, in parts per
  // million. The slope is host seconds per device second, so a fast device has a
  // slope a little under one, and the drift is (1 / slope - 1) times a million.
  double drift_ppm() const {
    // TODO
    return 0.0;
  }

 private:
  struct Slot {
    bool used = false;
    long long index = 0;
    double device = 0.0;
    double host = 0.0;
  };

  double bucket_seconds_;
  std::vector<Slot> slots_;
  bool any_ = false;
  long long newest_ = 0;
};

#endif  // LESSON_SOLUTION_HPP
