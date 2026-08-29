#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cstddef>
#include <map>
#include <optional>
#include <string>
#include <vector>

struct Reading {
  std::string name;
  double value = 0.0;
  long at_ms = 0;
};

class SensorLog {
 public:
  explicit SensorLog(std::size_t capacity) : capacity_(capacity == 0 ? 1 : capacity) {
    // TODO: make the one allocation this object will ever make. After the
    // constructor, recording must never allocate.
  }

  // Stores the reading as the latest for that sensor, and appends it to the
  // bounded history. Once the history is full, the oldest is overwritten.
  void record(const std::string& name, double value, long at_ms) {
    // TODO
    (void)name;
    (void)value;
    (void)at_ms;
  }

  std::optional<Reading> latest(const std::string& name) const {
    // TODO
    (void)name;
    return std::nullopt;
  }

  // Every sensor name seen, in sorted order. Choosing the right container for
  // latest_ makes this almost free rather than a sort on every call.
  std::vector<std::string> names() const {
    // TODO
    return {};
  }

  // The readings held, oldest first. Once the buffer has wrapped, the oldest is
  // not the one at index zero.
  std::vector<Reading> history() const {
    // TODO
    return {};
  }

  std::size_t size() const { return size_; }
  std::size_t capacity() const { return capacity_; }

 private:
  std::size_t capacity_ = 1;
  std::size_t size_ = 0;
  std::size_t next_ = 0;
  std::vector<Reading> history_;
  std::map<std::string, Reading> latest_;
};

#endif  // LESSON_SOLUTION_HPP
