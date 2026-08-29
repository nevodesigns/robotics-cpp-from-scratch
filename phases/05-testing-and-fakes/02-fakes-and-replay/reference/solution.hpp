#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cstddef>
#include <map>
#include <vector>

#include <rc/core/compat.hpp>

enum class ReadError {
  EndOfData,
  Disconnected,
  Corrupt,
  Timeout,
};

// What the code under test depends on. Not a device: an interface that a device
// or a recording can both satisfy.
class SensorSource {
 public:
  virtual ~SensorSource() = default;   // deleting through a base pointer without
                                       // this is undefined behaviour
  virtual rc::expected<double, ReadError> read(long at_ms) = 0;
};

// A fake that answers from a recorded trace, and can be told to fail at chosen
// points so the failure paths are reachable on demand.
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

 private:
  std::vector<double> trace_;
  std::map<std::size_t, ReadError> failures_;
  std::size_t reads_ = 0;
};

// The code under test. It knows nothing about ports, files or hardware.
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

    // A good reading clears the run. Counting total failures and consecutive
    // ones separately matters: a sensor that drops one reading an hour is
    // healthy, and one that drops five in a row is not.
    consecutive_failures_ = 0;
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

#endif  // LESSON_SOLUTION_HPP
