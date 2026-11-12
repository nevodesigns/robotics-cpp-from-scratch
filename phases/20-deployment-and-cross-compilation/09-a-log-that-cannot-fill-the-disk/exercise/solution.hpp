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
inline bool policy_valid(const LogPolicy& p) {
  // TODO: true if max_file_bytes > 0, max_files >= 1 and max_total_bytes >= max_file_bytes
  (void)p;
  return false;
}

// The most the log can take up in the steady state: the smaller of the total allowed and the number of files times the size of a file.
inline std::uint64_t budget(const LogPolicy& p) {
  // TODO: the smaller of max_total_bytes and max_files * max_file_bytes
  (void)p;
  return 0;
}

// The log files of a robot, as a record of sizes. Records are appended to the current file. When the next one would take it past max_file_bytes (and it holds something) it is closed and a new one started; then the oldest closed files are deleted,
// oldest first, while there are more than max_files files or the total is over max_total_bytes. The current file is never deleted, so one record bigger than the whole budget is kept until the next one arrives. A record of 0 bytes is ignored.
class LogStore {
 public:
  explicit LogStore(const LogPolicy& p) : policy_(p) {}

  void append(std::uint64_t bytes) {
    // TODO: ignore 0. If the current file holds something and this record would take it past max_file_bytes, close it (push its size on closed_) and start a new one. Add the record to current_ and total_. Then while there is a closed file and (files including the current would be over max_files, or total_ is over max_total_bytes): delete the oldest closed file (subtract it from total_, add to deleted_, first_seq_++)
    (void)bytes;
    (void)policy_;
    (void)closed_;
    (void)current_;
    (void)total_;
    (void)deleted_;
    (void)first_seq_;
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
    // TODO: the state of this id: if no window is open, or now is a window or more after its start, or now is before its start, open a new one at now with a count of 0. If count < limit_: count it and allow. Else suppressed++ and refuse
    (void)id;
    (void)now;
    (void)limit_;
    (void)window_;
    (void)states_;
    return false;
  }

  std::uint64_t suppressed(const std::string& id) const {
    auto it = states_.find(id);
    return it == states_.end() ? 0 : it->second.suppressed;
  }

  std::uint64_t take_suppressed(const std::string& id) {
    // TODO: the suppressed count of the id, reset to 0 (0 for an id never seen)
    (void)id;
    (void)states_;
    return 0;
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
