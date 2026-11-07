#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cstddef>
#include <deque>
#include <queue>
#include <vector>

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
    // TODO: build an Event with this time and id and seq = next_seq_++, and push it onto queue_.
    (void)time;
    (void)id;
    (void)next_seq_;
  }

  // Take the earliest event. The queue must not be empty.
  Event pop() {
    // TODO: copy queue_.top(), call queue_.pop(), return the copy.
    (void)queue_;
    return Event{};
  }

  bool empty() const { return true; }       // TODO: queue_.empty()
  std::size_t size() const { return 0; }    // TODO: queue_.size()

 private:
  std::priority_queue<Event, std::vector<Event>, Later> queue_;
  unsigned long next_seq_ = 0;
};

// The last `capacity` readings, and their mean.
class Window {
 public:
  explicit Window(std::size_t capacity) : capacity_(capacity > 0 ? capacity : 1) {}

  void add(double value) {
    // TODO: push_back the value and add it to sum_. If the window now holds more than capacity_,
    // take the front value off sum_ and pop_front it. Keep a running sum: adding up the whole
    // window every time is the slow way.
    (void)value;
    (void)capacity_;
    (void)values_;
    (void)sum_;
  }

  std::size_t size() const { return 0; }    // TODO: values_.size()

  double mean() const { return 0.0; }       // TODO: sum_ divided by the size, or 0 if empty

  double oldest() const { return 0.0; }     // TODO: the front value, or 0 if empty

 private:
  std::size_t capacity_;
  std::deque<double> values_;
  double sum_ = 0.0;
};

#endif  // LESSON_SOLUTION_HPP
