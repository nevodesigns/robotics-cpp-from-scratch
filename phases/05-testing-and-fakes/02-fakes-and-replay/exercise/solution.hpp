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

  void fail_at(std::size_t index, ReadError error) {
    // TODO: remember that this reading should fail with this error.
    (void)index;
    (void)error;
  }

  rc::expected<double, ReadError> read(long) override {
    // TODO
    //
    // Answer the next value from the trace, in order. Count the read even when
    // it fails, because a caller did ask. If this index was registered with
    // fail_at, report that error instead. Past the end of the trace, report
    // EndOfData.
    return rc::unexpected(ReadError::EndOfData);
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
    // TODO
    //
    // Read once. On failure, count it, count the consecutive run, and remember
    // the error. On success, clear the consecutive run, count it, and fold the
    // value into the running total.
    (void)source;
    (void)at_ms;
  }

  bool stale() const {
    // TODO: stale once more than allowed_failures readings have failed in a row.
    return false;
  }

  double mean() const {
    // TODO: the mean of the good readings, and 0.0 when there have been none.
    return 0.0;
  }

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
