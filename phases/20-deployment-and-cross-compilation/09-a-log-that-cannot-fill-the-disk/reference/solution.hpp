#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <algorithm>
#include <cstdint>
#include <deque>
#include <map>
#include <string>

struct LogPolicy {
  std::uint64_t max_file_bytes = 10u * 1024 * 1024;     // a file is closed and a new one started at about this size
  std::uint64_t max_total_bytes = 200u * 1024 * 1024;   // all the log files together are kept under this
  int max_files = 20;                                   // and there are never more than this many
};

// A policy is usable if a file may hold something, there is room for at least one file, and the total allows at least one file of the full size.
inline bool policy_valid(const LogPolicy& p) { return p.max_file_bytes > 0 && p.max_files >= 1 && p.max_total_bytes >= p.max_file_bytes; }

// The most the log can take up in the steady state: the smaller of the total allowed and the number of files times the size of a file.
inline std::uint64_t budget(const LogPolicy& p) {
  const std::uint64_t by_files = static_cast<std::uint64_t>(p.max_files > 0 ? p.max_files : 0) * p.max_file_bytes;
  return std::min(p.max_total_bytes, by_files);
}

// The log files of a robot, as a record of sizes. Records are appended to the current file. When the next one would take it past max_file_bytes (and it holds something) it is closed and a new one started; then the oldest closed files are deleted,
// oldest first, while there are more than max_files files or the total is over max_total_bytes. The current file is never deleted, so one record bigger than the whole budget is kept until the next one arrives. A record of 0 bytes is ignored.
class LogStore {
 public:
  explicit LogStore(const LogPolicy& p) : policy_(p) {}

  void append(std::uint64_t bytes) {
    if (bytes == 0) return;
    if (current_ > 0 && current_ + bytes > policy_.max_file_bytes) {
      closed_.push_back(current_);
      current_ = 0;
    }
    current_ += bytes;
    total_ += bytes;
    while (!closed_.empty() && (static_cast<int>(closed_.size()) + 1 > policy_.max_files || total_ > policy_.max_total_bytes)) {
      total_ -= closed_.front();
      deleted_ += closed_.front();
      closed_.pop_front();
      ++first_seq_;
    }
  }

  std::uint64_t total() const { return total_; }
  std::uint64_t deleted() const { return deleted_; }
  int files() const { return static_cast<int>(closed_.size()) + (current_ > 0 ? 1 : 0); }
  std::uint64_t first_seq() const { return first_seq_; }                                // the number of the oldest file kept, counting from 0
  std::uint64_t last_seq() const { return first_seq_ + closed_.size(); }               // and of the newest, the one being written

 private:
  LogPolicy policy_;
  std::deque<std::uint64_t> closed_;
  std::uint64_t current_ = 0;
  std::uint64_t total_ = 0;
  std::uint64_t deleted_ = 0;
  std::uint64_t first_seq_ = 0;
};

// Stops one message being written a million times. For each message id, a window opens at the first message seen; within `window` seconds of it, the first `limit` messages are allowed and the rest are suppressed and counted. A message that arrives after the window has closed opens a new one and is allowed.
// The count of suppressed messages is kept for the id until it is taken, so that a summary ("repeated N times") can be written.
class FloodGuard {
 public:
  FloodGuard(int limit, double window) : limit_(limit), window_(window) {}

  bool allow(const std::string& id, double now) {
    State& s = states_[id];
    if (!s.open || now - s.start >= window_ || now < s.start) {
      s.open = true;
      s.start = now;
      s.count = 0;
    }
    if (s.count < limit_) {
      ++s.count;
      return true;
    }
    ++s.suppressed;
    return false;
  }

  std::uint64_t suppressed(const std::string& id) const {
    auto it = states_.find(id);
    return it == states_.end() ? 0 : it->second.suppressed;
  }

  std::uint64_t take_suppressed(const std::string& id) {
    auto it = states_.find(id);
    if (it == states_.end()) return 0;
    const std::uint64_t n = it->second.suppressed;
    it->second.suppressed = 0;
    return n;
  }

 private:
  struct State {
    bool open = false;
    double start = 0.0;
    int count = 0;
    std::uint64_t suppressed = 0;
  };
  int limit_ = 5;
  double window_ = 10.0;
  std::map<std::string, State> states_;
};

#endif  // LESSON_SOLUTION_HPP
