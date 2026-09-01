// rc/core/log.hpp
//
// The bounded sensor log from lesson 03-03, graduated.
//
// Bounded because a log that grows without limit is a device that dies after a
// week of working perfectly. Which container to use is a question about the
// operations you actually perform, and lesson 03-03 measured them rather than
// quoting a table.

#ifndef RC_CORE_LOG_HPP
#define RC_CORE_LOG_HPP

#include <cstddef>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace rc {
namespace core {

struct Reading {
  std::string name;
  double value = 0.0;
  long at_ms = 0;
};

class SensorLog {
 public:
  explicit SensorLog(std::size_t capacity) : capacity_(capacity == 0 ? 1 : capacity) {
    // The one allocation this object ever makes. Everything afterwards writes
    // into memory that already exists, which is what keeps a control loop's
    // timing predictable.
    history_.resize(capacity_);
  }

  void record(const std::string& name, double value, long at_ms) {
    Reading reading;
    reading.name = name;
    reading.value = value;
    reading.at_ms = at_ms;

    // A map rather than an unordered_map, because names() wants sorted order
    // and getting it free from the container beats sorting on every call.
    latest_[name] = reading;

    // Overwrite in place. next_ walks round the block, and once the buffer is
    // full the oldest entry is simply the one about to be replaced.
    history_[next_] = std::move(reading);
    next_ = (next_ + 1) % capacity_;
    if (size_ < capacity_) ++size_;
  }

  std::optional<Reading> latest(const std::string& name) const {
    const auto found = latest_.find(name);
    if (found == latest_.end()) return std::nullopt;
    return found->second;
  }

  std::vector<std::string> names() const {
    std::vector<std::string> out;
    out.reserve(latest_.size());
    // A map iterates in key order, so this is already sorted.
    for (const auto& entry : latest_) out.push_back(entry.first);
    return out;
  }

  // Oldest first. When the buffer has wrapped, the oldest entry is the one that
  // next_ is about to overwrite, not the one at index zero.
  std::vector<Reading> history() const {
    std::vector<Reading> out;
    out.reserve(size_);

    const std::size_t oldest = (size_ < capacity_) ? 0 : next_;
    for (std::size_t i = 0; i < size_; ++i) {
      out.push_back(history_[(oldest + i) % capacity_]);
    }
    return out;
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

}  // namespace core
}  // namespace rc

#endif  // RC_CORE_LOG_HPP
