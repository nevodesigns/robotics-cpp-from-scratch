#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cstddef>
#include <cstdint>
#include <deque>
#include <map>
#include <set>
#include <vector>

// A link that drops messages needs a sender that does not give up on the first loss. The sender keeps each command until the robot has said it arrived, and sends it again
// if the answer does not come in time. That is correct, and it creates a second problem: the robot may have received the command, and it was the ACKNOWLEDGEMENT that was
// lost. Now the command arrives twice, and a command to move one metre, applied twice, is two metres.
//
// The fix is on the receiving side. Each command carries a sequence number, and the receiver applies a number it has not seen and only acknowledges one it has. Sending again
// is safe because applying twice is not possible.

// The sender: which commands are waiting for an answer, when each is to be sent again, and which gave up.
class CommandSender {
 public:
  // A command is sent again if it is not acknowledged within `retry_ms`, and fails when it has been sent `max_attempts` times and that time has passed again.
  CommandSender(long long retry_ms, int max_attempts) : retry_ms_(retry_ms), max_attempts_(max_attempts < 1 ? 1 : max_attempts) {}

  // Starts a new command and returns its sequence number, which wraps at 65536. The caller sends it now: this is the first attempt.
  std::uint16_t send(long long now_ms) {
    const std::uint16_t seq = next_++;
    Entry e;
    e.attempts = 1;
    e.due_ms = now_ms + retry_ms_;
    pending_[seq] = e;
    return seq;
  }

  // The commands to send again at `now_ms`, in sequence order. Each counts as another attempt and is due again `retry_ms` later. A command that has been sent
  // `max_attempts` times and is due again is not sent: it is removed, and appears in failed().
  std::vector<std::uint16_t> due(long long now_ms) {
    std::vector<std::uint16_t> again;
    for (auto it = pending_.begin(); it != pending_.end();) {
      if (it->second.due_ms > now_ms) {
        ++it;
        continue;
      }
      if (it->second.attempts >= max_attempts_) {
        failed_.push_back(it->first);
        it = pending_.erase(it);
        continue;
      }
      ++it->second.attempts;
      it->second.due_ms = now_ms + retry_ms_;
      again.push_back(it->first);
      ++it;
    }
    return again;
  }

  // The robot said it has the command with this number. Returns true if it was waiting, and false for one that is not (already answered, failed, or never sent).
  bool ack(std::uint16_t seq) { return pending_.erase(seq) > 0; }

  std::size_t pending() const { return pending_.size(); }
  const std::vector<std::uint16_t>& failed() const { return failed_; }
  std::uint16_t next_number() const { return next_; }   // the number the next command will have

 private:
  struct Entry {
    int attempts = 0;
    long long due_ms = 0;
  };

  long long retry_ms_;
  int max_attempts_;
  std::uint16_t next_ = 0;
  std::map<std::uint16_t, Entry> pending_;
  std::vector<std::uint16_t> failed_;
};

// The receiver: applies a command the first time its number is seen, and only recognises it after. It remembers the last `window` numbers.
class CommandReceiver {
 public:
  explicit CommandReceiver(std::size_t window) : window_(window < 1 ? 1 : window) {}

  // True the first time this number is seen within the window, and false for a number that is remembered. The caller acknowledges in both cases.
  bool accept(std::uint16_t seq) {
    if (seen_.count(seq) > 0) return false;
    seen_.insert(seq);
    order_.push_back(seq);
    if (order_.size() > window_) {
      seen_.erase(order_.front());
      order_.pop_front();
    }
    return true;
  }

 private:
  std::size_t window_;
  std::set<std::uint16_t> seen_;
  std::deque<std::uint16_t> order_;
};

#endif  // LESSON_SOLUTION_HPP
