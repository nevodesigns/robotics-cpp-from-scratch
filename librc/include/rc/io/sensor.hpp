// rc/io/sensor.hpp
//
// The fake device pattern from lesson 05-02, graduated.
//
// The single idea every driver lesson in this curriculum depends on: code under
// test depends on an interface, not on a device. A real port and a recorded
// trace both satisfy it, so the logic can be tested at full speed, on a machine
// with no hardware attached, including the failure paths that a real sensor
// will not produce on demand.
//
// A recording is worth more than a mock here. A mock asserts that the code
// asked for what you expected, which is a claim about the code. A replay
// answers with what the hardware actually said on the day it was captured,
// which is a claim about the world, and the second is what catches the bug.

#ifndef RC_IO_SENSOR_HPP
#define RC_IO_SENSOR_HPP

#include <cstddef>
#include <map>
#include <utility>
#include <vector>

#include <rc/core/compat.hpp>

namespace rc {
namespace io {

enum class ReadError {
  EndOfData,
  Disconnected,
  Corrupt,
  Timeout,
};

// What the code under test depends on. Not a device: an interface a device or a
// recording can both satisfy.
class SensorSource {
 public:
  // Without a virtual destructor, deleting through a base pointer is undefined
  // behaviour, and the usual symptom is a derived member quietly never
  // destroyed rather than anything as helpful as a crash.
  virtual ~SensorSource() = default;

  virtual rc::expected<double, ReadError> read(long at_ms) = 0;
};

// Answers from a recorded trace, and can be told to fail at chosen points so
// the failure paths are reachable on demand rather than by unplugging a cable
// at the right moment.
class ReplaySensor : public SensorSource {
 public:
  explicit ReplaySensor(std::vector<double> trace) : trace_(std::move(trace)) {}

  void fail_at(std::size_t index, ReadError error) { failures_[index] = error; }

  rc::expected<double, ReadError> read(long) override {
    const std::size_t index = reads_;
    ++reads_;   // counted even when the read fails, because a caller did ask

    const auto planned = failures_.find(index);
    if (planned != failures_.end()) return rc::unexpected(planned->second);

    if (index >= trace_.size()) return rc::unexpected(ReadError::EndOfData);
    return trace_[index];
  }

  std::size_t reads() const { return reads_; }
  void rewind() { reads_ = 0; }

 private:
  std::vector<double> trace_;
  std::map<std::size_t, ReadError> failures_;
  std::size_t reads_ = 0;
};

// Health of a sensor stream, kept as the readings arrive.
//
// Total failures and consecutive failures are counted separately because they
// answer different questions. A sensor that drops one reading an hour is
// healthy. One that drops five in a row is not, and a single counter cannot
// tell those apart.
class SensorMonitor {
 public:
  explicit SensorMonitor(int allowed_failures) : allowed_failures_(allowed_failures) {}

  void poll(SensorSource& source, long at_ms) {
    const auto reading = source.read(at_ms);

    if (!reading.has_value()) {
      ++failures_;
      ++consecutive_failures_;
      last_error_ = reading.error();
      return;
    }

    consecutive_failures_ = 0;   // a good reading clears the run
    ++good_;
    sum_ += reading.value();
  }

  bool stale() const { return consecutive_failures_ > allowed_failures_; }

  double mean() const { return good_ == 0 ? 0.0 : sum_ / static_cast<double>(good_); }

  int good() const { return good_; }
  int failures() const { return failures_; }
  int consecutive_failures() const { return consecutive_failures_; }
  ReadError last_error() const { return last_error_; }

 private:
  int allowed_failures_ = 0;
  int good_ = 0;
  int failures_ = 0;
  int consecutive_failures_ = 0;
  double sum_ = 0.0;
  ReadError last_error_ = ReadError::EndOfData;
};

}  // namespace io
}  // namespace rc

#endif  // RC_IO_SENSOR_HPP
