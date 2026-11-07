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
  //
  // Times are whole nanoseconds. A period of a tenth of a second is 100000000, and three of them is
  // exactly 300000000. As a double, 0.1 + 0.1 + 0.1 is 0.30000000000000004, which is later than 0.3.
  void every(Nanoseconds period, Nanoseconds first, Task task) {
    if (period <= 0) return;
    entries_.push_back(Entry{std::move(task), period});
    push(first, static_cast<int>(entries_.size() - 1));
  }

  // Run every event whose time is at or before `end`, earliest first, and leave the clock at `end`.
  void run_until(Nanoseconds end) {
    // The top of the queue is the earliest event, and looking at it does not disturb the order of the others.
    while (!queue_.empty() && static_cast<Nanoseconds>(queue_.top().time) <= end) {
      const Event e = queue_.top();
      queue_.pop();
      const Nanoseconds when = static_cast<Nanoseconds>(e.time);
      clock_->set(when);

      // Copy what is needed before the call: the task may add entries, and that can move the vector.
      const std::size_t index = static_cast<std::size_t>(e.id);
      const Nanoseconds period = entries_[index].period;
      Task task = entries_[index].task;
      task(when);
      ++events_run_;

      if (period > 0) push(when + period, e.id);
    }
    clock_->set(end);
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
