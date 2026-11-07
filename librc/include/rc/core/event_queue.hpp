// rc/core/event_queue.hpp
//
// What happens next, and a window of what happened, from lesson 03-16, graduated.
//
// std::priority_queue gives the largest first, so a schedule has to compare the other way round: with a plain one, the events 5, 1, 9 and 3 came out
// 9 first. Equal times need a tiebreak the queue does not provide: eight events at one time came out in scheduling order from this queue, and in the
// order 0 2 6 5 7 4 1 3 from a priority_queue comparing only the time (libstdc++, and not promised). Ten thousand events over 500 times came out exactly sorted
// with ties in order. A bounded window wants a deque: keeping 1000 of 5000 readings by erasing the front of a vector moved 4000000 elements, and pop_front
// on a deque moved 0.

#ifndef RC_CORE_EVENT_QUEUE_HPP
#define RC_CORE_EVENT_QUEUE_HPP

#include <cstddef>
#include <deque>
#include <queue>
#include <vector>

namespace rc {
namespace core {

// Something that should happen at a time. `seq` is the order it was scheduled in.
struct Event {
  double time = 0.0;
  unsigned long seq = 0;
  int id = 0;
};

// "a comes after b". std::priority_queue gives the LARGEST by its comparison, so asking for the earliest
// time means comparing the other way round: the queue's top is the one that is not later than any other.
struct Later {
  bool operator()(const Event& a, const Event& b) const {
    if (a.time != b.time) return a.time > b.time;
    return a.seq > b.seq;     // equal times: the one scheduled first is earlier
  }
};

// Events come out earliest first, and events at the same time come out in the order they went in.
class EventQueue {
 public:
  void schedule(double time, int id) {
    Event e;
    e.time = time;
    e.seq = next_seq_++;
    e.id = id;
    queue_.push(e);
  }

  // Take the earliest event. The queue must not be empty.
  Event pop() {
    const Event e = queue_.top();
    queue_.pop();
    return e;
  }

  bool empty() const { return queue_.empty(); }
  std::size_t size() const { return queue_.size(); }

 private:
  std::priority_queue<Event, std::vector<Event>, Later> queue_;
  unsigned long next_seq_ = 0;
};

// The last `capacity` readings, and their mean.
//
// A deque, because the oldest reading leaves from the front as the newest arrives at the back, and a
// deque does both without moving anything else.
class Window {
 public:
  explicit Window(std::size_t capacity) : capacity_(capacity > 0 ? capacity : 1) {}

  void add(double value) {
    values_.push_back(value);
    sum_ += value;
    if (values_.size() > capacity_) {
      sum_ -= values_.front();
      values_.pop_front();
    }
  }

  std::size_t size() const { return values_.size(); }

  double mean() const { return values_.empty() ? 0.0 : sum_ / static_cast<double>(values_.size()); }

  double oldest() const { return values_.empty() ? 0.0 : values_.front(); }

 private:
  std::size_t capacity_;
  std::deque<double> values_;
  double sum_ = 0.0;
};

}  // namespace core
}  // namespace rc

#endif  // RC_CORE_EVENT_QUEUE_HPP
