#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cstddef>
#include <functional>
#include <queue>
#include <utility>
#include <vector>

#include <rc/core/clock.hpp>
#include <rc/core/event_queue.hpp>

using rc::core::Event;
using rc::core::Later;
using rc::core::Nanoseconds;
using rc::core::TestClock;

// Runs tasks at times, in time order, on a clock the test controls.
//
// Nothing here waits. "Run until ten seconds" takes the next event, sets the clock to its time, runs it,
// and takes the next, so ten minutes of a thousand hertz loop is six hundred thousand function calls
// and no sleeping.
class Scheduler {
 public:
  using Task = std::function<void(Nanoseconds now)>;

  explicit Scheduler(TestClock* clock) : clock_(clock) {}

  // Run `task` once, at `when`.
  void at(Nanoseconds when, Task task) {
    entries_.push_back(Entry{std::move(task), 0});
    push(when, static_cast<int>(entries_.size() - 1));
  }

  // Run `task` at `first`, then every `period` after that.
  void every(Nanoseconds period, Nanoseconds first, Task task) {
    // TODO: a period of zero or less does nothing. Otherwise add Entry{std::move(task), period} to
    // entries_ and push(first, index of the new entry).
    (void)period;
    (void)first;
    (void)task;
  }

  // Run every event whose time is at or before `end`, earliest first, and leave the clock at `end`.
  void run_until(Nanoseconds end) {
    // TODO: while the queue is not empty and the top event's time (a double holding whole
    // nanoseconds, so cast it to Nanoseconds) is at or before `end`:
    //   - copy queue_.top() and pop it;
    //   - set the clock to the event's time with clock_->set(...);
    //   - copy the entry's period and task out of entries_ BEFORE calling it (the task may add
    //     entries, and that can move the vector);
    //   - call task(when), then count it in events_run_;
    //   - if the period is above zero, push(when + period, the event's id).
    // After the loop, clock_->set(end).
    (void)end;
    (void)clock_;
    (void)queue_;
    (void)events_run_;
  }

  long events_run() const { return events_run_; }

 private:
  struct Entry {
    Task task;
    Nanoseconds period;   // zero for a task that runs once
  };

  void push(Nanoseconds when, int id) {
    Event e;
    e.time = static_cast<double>(when);      // whole nanoseconds are exact in a double up to about 104 days
    // Ties at one instant are broken by the order the tasks were registered, not by when this event was
    // queued. A repeating task queues its next tick as it finishes the last, so by queue order the
    // task registered last would often win.
    e.seq = static_cast<unsigned long>(id);
    e.id = id;
    queue_.push(e);
  }

  TestClock* clock_;
  std::priority_queue<Event, std::vector<Event>, Later> queue_;
  std::vector<Entry> entries_;
  long events_run_ = 0;
};

#endif  // LESSON_SOLUTION_HPP
