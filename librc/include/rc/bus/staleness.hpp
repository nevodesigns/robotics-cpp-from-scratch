// rc/bus/staleness.hpp
//
// Noticing silence and refusing stale data, from lesson 17-08, graduated.
//
// A deadline monitor reports a silence longer than the deadline once, however long it lasts, and is armed again by the next message; nothing is missed before the first. A queue whose messages expire hands over only
// those still good and counts the rest.
//
// A publisher that died after its last message at 1.0 s was noticed at 1.26 s with a deadline of 0.25 s. With a message every 0.10 s, each up to 0.03 s early or late, 2000 messages gave 992, 441, 124 and 0 misses
// at deadlines of 0.10, 0.12, 0.14 and 0.17 s. A consumer taking half as fast as messages were published served data 5.00 s old with no lifespan, 0.50 s with a lifespan of 0.5 s and 0.10 s with 0.1 s.

#ifndef RC_BUS_STALENESS_HPP
#define RC_BUS_STALENESS_HPP

#include <cstddef>
#include <deque>
#include <string>

namespace rc {
namespace bus {

// Watches a stream that promised to send at least once every `deadline` seconds. It is told when messages arrive and asked, from time to time,
// whether the promise has been broken. A broken promise is reported once for each silence, however long, and not again until a message arrives.
class DeadlineMonitor {
 public:
  explicit DeadlineMonitor(double deadline) : deadline_(deadline) {}

  void on_message(double now) {
    if (started_ && now - last_ > longest_gap_) longest_gap_ = now - last_;
    last_ = now;
    started_ = true;
    reported_ = false;
  }

  // The number of deadlines newly found missed: 1 if the silence since the last message is longer than the deadline and has not been reported, otherwise 0.
  // Nothing is missed before the first message.
  int check(double now) {
    if (!started_ || reported_) return 0;
    if (now - last_ > deadline_) {
      reported_ = true;
      ++missed_;
      return 1;
    }
    return 0;
  }

  int missed() const { return missed_; }
  double longest_gap() const { return longest_gap_; }

 private:
  double deadline_ = 1.0;
  double last_ = 0.0;
  bool started_ = false;
  bool reported_ = false;
  int missed_ = 0;
  double longest_gap_ = 0.0;
};

// A queue of messages that stop being worth delivering `lifespan` seconds after they were made. Taking skips the ones that have expired
// and counts them. A lifespan of zero or less means they never expire.
class ExpiringQueue {
 public:
  explicit ExpiringQueue(double lifespan) : lifespan_(lifespan) {}

  void push(const std::string& payload, double stamp) { items_.push_back(Item{payload, stamp}); }

  // The oldest message that is still good at time `now`, with the age it has reached. False if none is left.
  bool take(double now, std::string& payload, double& age) {
    while (!items_.empty()) {
      const Item item = items_.front();
      items_.pop_front();
      const double a = now - item.stamp;
      if (lifespan_ > 0.0 && a > lifespan_) {
        ++expired_;
        continue;
      }
      payload = item.payload;
      age = a;
      return true;
    }
    return false;
  }

  std::size_t size() const { return items_.size(); }
  int expired() const { return expired_; }

 private:
  struct Item {
    std::string payload;
    double stamp = 0.0;
  };
  double lifespan_ = 0.0;
  std::deque<Item> items_;
  int expired_ = 0;
};

}  // namespace bus
}  // namespace rc

#endif  // RC_BUS_STALENESS_HPP
