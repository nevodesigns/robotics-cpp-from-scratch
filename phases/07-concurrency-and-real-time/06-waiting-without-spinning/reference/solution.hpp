#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <deque>
#include <mutex>

// Lesson 07-02 built a queue whose pop returns false when it is empty, and left
// the next question unasked: what does the consumer do then?
//
// The answer most people write first is a loop. Ask again, and again, until
// something is there. It works, and it is measured below to cost a whole core.
//
// Measured once, outside the suite, on a 12 core machine with gcc 11. A consumer
// waits 100 ms for a producer, three ways:
//
//                            wakeups   CPU the consumer used, of 100 ms
//   spin, with yield          231000 to 309000          100.05 ms
//   poll every 1 ms                92                     1.85 ms
//   condition variable              1                     0.04 ms
//
// Spinning used the whole core to find out that nothing had happened. Polling
// every millisecond is cheap but is a wakeup per millisecond for as long as
// nothing happens, and an item that arrives just after a poll waits for the next
// one. The condition variable woke once, when there was something to do.
//
// Time is a property of the machine that day, so the suite asserts the counts
// and the CPU numbers above are the one-off measurement that motivates it.
//
// A condition variable is a way to sleep until another thread says something
// changed. It has two rules and both are the same rule: the thing you are waiting
// for is STATE, kept under a mutex, and the condition variable only says "look
// again".
//
// A notification is not remembered. Measured: notify_one with nobody waiting, then
// wait_for 50 ms with no predicate, timed out. The notification was gone. The
// same wait, asking whether a flag was set first, returned at once. That is the
// whole reason to wait on a predicate and never on the notification alone.
//
// A wakeup does not mean the condition is true. Another consumer may have taken
// the item first, and the standard allows a wait to return with nobody having
// notified at all, a spurious wakeup. So the wait is always a loop that checks
// the state again:
//
//   while (items_.empty() && !closed_) wait(...);
//
// Measured: two consumers waiting, one item pushed, notify_all. One took it, the
// other woke, found nothing, and went back to sleep. Then close(): the other
// returned "closed".
//
// A thread asleep on a condition variable cannot be told to stop except by the
// condition variable. A queue with no close() leaves its consumers asleep for
// ever, and the program cannot exit while a thread it started is still waiting.
// close() sets the state and wakes EVERYONE, because every waiter has to notice.
//
// Every wait here has a deadline. A control path that waits for ever on a producer
// that has died has stopped controlling, and a test that waits for ever on a queue
// that was never written is a hung test. pop_for is the only way to take an item,
// and a timeout is a result like any other.
//
// The rule that falls out: keep the condition in state under the mutex, wait in a
// loop that rechecks it, notify after changing it, close by waking everyone, and
// never wait without a deadline.

template <typename T>
class BlockingQueue {
 public:
  enum class Result { kGot, kClosed, kTimeout };

  // Adds an item and wakes one waiting consumer.
  //
  // The state changes under the lock. The notification is sent after it is
  // released, so the thread that wakes is not immediately blocked on a mutex
  // this thread still holds.
  void push(const T& item) {
    {
      const std::lock_guard<std::mutex> held(mutex_);
      items_.push_back(item);
    }
    ready_.notify_one();
  }

  // Takes the oldest item, waiting up to timeout for one to arrive.
  //
  // kGot with the item in out; kClosed when the queue is closed and empty;
  // kTimeout when the time ran out first. Items already queued when close() is
  // called are still handed out, so a shutdown does not drop work.
  Result pop_for(T& out, std::chrono::milliseconds timeout) {
    const auto deadline = std::chrono::steady_clock::now() + timeout;
    std::unique_lock<std::mutex> lock(mutex_);

    // A loop and not an if. A wakeup says look again, and the answer may still
    // be no: another consumer got there first, or the wait returned for no
    // reason at all.
    while (items_.empty() && !closed_) {
      ++waits_;
      if (ready_.wait_until(lock, deadline) == std::cv_status::timeout) break;
    }

    // Checked after the loop, whichever way it ended: an item that arrived at the
    // instant of the timeout is still an item.
    if (!items_.empty()) {
      out = items_.front();
      items_.pop_front();
      return Result::kGot;
    }
    return closed_ ? Result::kClosed : Result::kTimeout;
  }

  // Marks the queue finished and wakes every waiter, not just one: each of them
  // has to notice, and notify_one would leave the rest asleep.
  void close() {
    {
      const std::lock_guard<std::mutex> held(mutex_);
      closed_ = true;
    }
    ready_.notify_all();
  }

  // Never waits. For the poller in the suite, and for a caller who has other
  // work to do.
  bool try_pop(T& out) {
    const std::lock_guard<std::mutex> held(mutex_);
    if (items_.empty()) return false;
    out = items_.front();
    items_.pop_front();
    return true;
  }

  std::size_t size() const {
    const std::lock_guard<std::mutex> held(mutex_);
    return items_.size();
  }

  // How many times a consumer has gone to sleep, which is how a test knows a
  // thread is really waiting before it acts, and how many times one woke.
  int waits() const {
    const std::lock_guard<std::mutex> held(mutex_);
    return waits_;
  }

 private:
  mutable std::mutex mutex_;
  std::condition_variable ready_;
  std::deque<T> items_;
  bool closed_ = false;
  int waits_ = 0;
};

#endif  // LESSON_SOLUTION_HPP
