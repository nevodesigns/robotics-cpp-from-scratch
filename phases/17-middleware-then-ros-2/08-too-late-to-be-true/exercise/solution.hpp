#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cstddef>
#include <deque>
#include <string>

// Watches a stream that promised to send at least once every `deadline` seconds. It is told when messages arrive and asked, from time to time,
// whether the promise has been broken. A broken promise is reported once for each silence, however long, and not again until a message arrives.
class DeadlineMonitor {
 public:
  explicit DeadlineMonitor(double deadline) : deadline_(deadline) {}

  void on_message(double now) {
    // TODO: If a message has arrived before, the gap since it (now - last_) is the new longest_gap_ if it is bigger than the one held.
    // Then last_ = now, started_ = true and reported_ = false: a message re-arms the monitor.
    (void)now;
    (void)longest_gap_;
    (void)last_;
    (void)started_;
    (void)reported_;
  }

  // The number of deadlines newly found missed: 1 if the silence since the last message is longer than the deadline and has not been reported, otherwise 0.
  // Nothing is missed before the first message.
  int check(double now) {
    // TODO: 0 if no message has arrived yet, or this silence has already been reported. Otherwise, if now - last_ is more than deadline_:
    // mark it reported, add one to missed_ and return 1. In every other case return 0.
    (void)now;
    (void)deadline_;
    (void)missed_;
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
    // TODO: While there are items: take the oldest off the front; its age is now - stamp. If lifespan_ is above zero and the age is over it, count it
    // in expired_ and carry on with the next. Otherwise give its payload and age and return true. False if there are none left.
    (void)now;
    (void)payload;
    (void)age;
    (void)lifespan_;
    (void)expired_;
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

#endif  // LESSON_SOLUTION_HPP
