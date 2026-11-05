// rc/io/sim_device.hpp
//
// A device you do not own, graduated from the capstone of phase 08.
//
// Most of this curriculum's robot hardware is not on the learner's desk, and the
// code that talks to it still has to be tested. SimDevice is the other end of a
// link: it streams numbered, time stamped telemetry at a steady rate, answers
// requests, remembers the last one it answered so a repeat is replayed, and can be
// unplugged, power cycled, made to answer late or not at all, and starved of the
// buffer a serial driver would have.
//
// It runs on the host's clock, which the caller passes to step(), so a test can
// make an hour pass in a moment and get the same answer every time. Its own clock
// is wrong on purpose: it runs fast by a configurable amount and stamps with
// sixteen bits of milliseconds, which wraps.
//
// Wire format. Every message is a frame as rc/io/link.hpp builds it.
//
//   telemetry  02  sequence(u16)  ticks(u16)  position(i32)
//   request    01  id  operation  arguments...
//   response   81  id  status  position(i32)
//
// Operation 0x10 moves the position by an i16. Operation 0x12 does nothing and
// answers, which is a ping.

#ifndef RC_IO_SIM_DEVICE_HPP
#define RC_IO_SIM_DEVICE_HPP

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <vector>

#include <rc/core/compat.hpp>
#include <rc/io/bytes.hpp>
#include <rc/io/checksum.hpp>
#include <rc/io/link.hpp>
#include <rc/io/request.hpp>
#include <rc/io/rescan.hpp>

namespace rc {
namespace io {

constexpr std::uint8_t kTelemetryType = 0x02;
constexpr std::uint8_t kOpMove = 0x10;
constexpr std::uint8_t kOpPing = 0x12;

struct SimDeviceOptions {
  double telemetry_hz = 100.0;
  double clock_ppm = 50.0;          // how much faster than the host's its own clock runs
  std::uint32_t seed = 12345;
  double min_latency = 0.002;       // seconds a message takes at the least
  double jitter = 0.008;            // and up to this much more
  std::size_t rx_capacity = 4096;   // what the host side can hold before new bytes are lost
  int late_every = 0;               // every nth new request is answered after late_by seconds, 0 for never
  double late_by = 0.08;
  int drop_every = 0;               // and every nth has its answer lost, 0 for never
  bool remembers = true;            // a ReplyCache, so a repeated request is replayed and not repeated
};

class SimDevice {
 public:
  explicit SimDevice(SimDeviceOptions options = SimDeviceOptions{})
      : options_(options), parser_(Check::Crc8, rc::span<std::uint8_t>(window_, sizeof(window_))),
        state_(options.seed == 0 ? 1u : options.seed) {}

  // Brings the device up to the time given. Telemetry that was due is made, bytes
  // from the host are read, and whatever has finished crossing the link is made
  // available to take_tx().
  void step(double now) {
    now_ = now;

    while (next_telemetry_ <= now) {
      send_telemetry(next_telemetry_);
      next_telemetry_ += 1.0 / options_.telemetry_hz;
    }

    for (std::size_t i = 0; i < rx_.size(); ++i) {
      bool ready = parser_.push(rx_[i]);
      while (ready) {
        handle_request(parser_.payload());
        ready = parser_.next();
      }
    }
    rx_.clear();

    while (!inflight_.empty() && inflight_.front().arrives <= now) {
      deliver(inflight_.front().bytes);
      inflight_.pop_front();
    }
  }

  // What the host sent. Lost when the cable is out.
  void put_rx(rc::span<const std::uint8_t> bytes) {
    if (!connected_) return;
    for (std::size_t i = 0; i < bytes.size(); ++i) rx_.push_back(bytes[i]);
  }

  // What has reached the host's end of the cable.
  std::size_t take_tx(rc::span<std::uint8_t> out) {
    std::size_t count = 0;
    while (count < out.size() && !tx_.empty()) {
      out[count++] = tx_.front();
      tx_.pop_front();
    }
    return count;
  }

  // The cable. Pulled out, nothing reaches the host and nothing the host sends
  // arrives, and what was on its way is gone.
  void set_connected(bool connected) {
    connected_ = connected;
    if (!connected) {
      inflight_.clear();
      tx_.clear();
      rx_.clear();
    }
  }
  bool connected() const { return connected_; }

  // The device is power cycled: its counter, clock, position and memory all
  // start again. Nothing is sent while it is down, because the caller does not
  // step it.
  void reboot(double now) {
    sequence_ = 0;
    position_ = 0;
    boot_time_ = now;
    boot_ticks_ = 120;
    next_telemetry_ = now + 1.0 / options_.telemetry_hz;
    cache_ = ReplyCache();
    distinct_ = 0;
    seen_any_ = false;
    inflight_.clear();
    tx_.clear();
    rx_.clear();
    parser_ = RescanParser(Check::Crc8, rc::span<std::uint8_t>(window_, sizeof(window_)));
    ++reboots_;
  }

  long position() const { return position_; }
  long executions() const { return executions_; }
  long replays() const { return replays_; }
  std::size_t telemetry_sent() const { return telemetry_sent_; }
  std::size_t overflowed() const { return overflowed_; }
  std::size_t reboots() const { return reboots_; }

 private:
  struct Inflight {
    double arrives = 0.0;
    std::vector<std::uint8_t> bytes;
  };

  std::uint32_t next_random() {
    state_ ^= state_ << 13;
    state_ ^= state_ >> 17;
    state_ ^= state_ << 5;
    return state_;
  }

  static std::vector<std::uint8_t> framed(const std::vector<std::uint8_t>& body) {
    std::vector<std::uint8_t> out{Link::kStartByte, static_cast<std::uint8_t>(body.size() + 1)};
    out.insert(out.end(), body.begin(), body.end());
    out.push_back(crc8(rc::span<const std::uint8_t>(body.data(), body.size())));
    return out;
  }

  std::uint16_t ticks_at(double t) const {
    const double device_ms = (t - boot_time_) * 1000.0 * (1.0 + options_.clock_ppm * 1e-6) +
                             static_cast<double>(boot_ticks_);
    return static_cast<std::uint16_t>(static_cast<std::uint64_t>(device_ms) & 0xFFFFu);
  }

  void schedule(double earliest, const std::vector<std::uint8_t>& bytes) {
    // A serial link delivers in order, so a message cannot overtake the one
    // before it, however its delay was drawn.
    const double arrives = std::max(earliest, last_arrival_);
    last_arrival_ = arrives;
    inflight_.push_back(Inflight{arrives, bytes});
  }

  void send_telemetry(double at) {
    std::vector<std::uint8_t> body(9, 0);
    body[0] = kTelemetryType;
    ByteWriter writer{rc::span<std::uint8_t>(body.data() + 1, 8)};
    writer.put_u16(static_cast<std::uint16_t>(sequence_++));
    writer.put_u16(ticks_at(at));
    writer.put_u32(static_cast<std::uint32_t>(position_));
    ++telemetry_sent_;

    const double delay = options_.min_latency + options_.jitter * static_cast<double>(next_random() % 1000u) / 1000.0;
    schedule(at + delay, framed(body));
  }

  void handle_request(rc::span<const std::uint8_t> message) {
    if (message.size() < 3 || message[0] != kRequestType) return;
    const std::uint8_t id = message[1];
    const std::uint8_t operation = message[2];

    const bool repeat = seen_any_ && id == last_id_;
    std::vector<std::uint8_t> body;

    rc::span<const std::uint8_t> remembered;
    if (options_.remembers && cache_.lookup(id, remembered)) {
      body.assign(remembered.data(), remembered.data() + remembered.size());
      ++replays_;
    } else {
      if (operation == kOpMove && message.size() >= 5) {
        ByteReader reader{rc::span<const std::uint8_t>(message.data() + 3, 2)};
        std::uint16_t raw = 0;
        reader.get_u16(raw);
        position_ += static_cast<std::int16_t>(raw);
      }
      ++executions_;
      body.push_back(0);
      std::uint8_t pos[4] = {};
      ByteWriter writer{rc::span<std::uint8_t>(pos, sizeof(pos))};
      writer.put_u32(static_cast<std::uint32_t>(position_));
      body.insert(body.end(), pos, pos + 4);
      if (options_.remembers) cache_.remember(id, rc::span<const std::uint8_t>(body.data(), body.size()));
    }

    std::vector<std::uint8_t> response{kResponseType, id};
    response.insert(response.end(), body.begin(), body.end());

    if (!repeat) {
      ++distinct_;
      seen_any_ = true;
    }
    last_id_ = id;

    double due = now_ + 0.005;
    bool lost = false;
    if (!repeat) {
      if (options_.late_every > 0 && distinct_ % options_.late_every == 0) due = now_ + options_.late_by;
      else if (options_.drop_every > 0 && distinct_ % options_.drop_every == 0) lost = true;
    }
    if (!lost) schedule(due, framed(response));
  }

  void deliver(const std::vector<std::uint8_t>& bytes) {
    if (!connected_) return;
    for (const std::uint8_t byte : bytes) {
      if (tx_.size() >= options_.rx_capacity) {
        ++overflowed_;
        continue;
      }
      tx_.push_back(byte);
    }
  }

  SimDeviceOptions options_;
  std::uint8_t window_[260] = {};
  RescanParser parser_;
  std::uint32_t state_;
  bool connected_ = true;
  double now_ = 0.0;
  double boot_time_ = 0.0;
  std::uint32_t boot_ticks_ = 777;
  double next_telemetry_ = 0.0;
  double last_arrival_ = 0.0;
  std::uint32_t sequence_ = 0;
  long position_ = 0;
  long executions_ = 0;
  long replays_ = 0;
  long distinct_ = 0;
  bool seen_any_ = false;
  std::uint8_t last_id_ = 0;
  std::size_t telemetry_sent_ = 0;
  std::size_t overflowed_ = 0;
  std::size_t reboots_ = 0;
  ReplyCache cache_;
  std::deque<std::uint8_t> rx_;
  std::deque<std::uint8_t> tx_;
  std::deque<Inflight> inflight_;
};

// The host's end of a SimDevice's cable, as a BytePort: what a SerialPort is to a
// real one.
class SimDevicePort : public BytePort {
 public:
  explicit SimDevicePort(SimDevice& device) : device_(device) {}

  int read(rc::span<std::uint8_t> into) override {
    if (!device_.connected()) return -1;
    return static_cast<int>(device_.take_tx(into));
  }

  int write(rc::span<const std::uint8_t> from) override {
    if (!device_.connected()) return -1;
    device_.put_rx(from);
    return static_cast<int>(from.size());
  }

 private:
  SimDevice& device_;
};

}  // namespace io
}  // namespace rc

#endif  // RC_IO_SIM_DEVICE_HPP
