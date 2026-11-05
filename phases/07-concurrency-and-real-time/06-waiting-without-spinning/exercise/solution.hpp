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

  // TODO 1: add an item and wake one waiting consumer.
  //
  // Change the state under the lock, in a block of its own so the lock is
  // released at the closing brace. Then call ready_.notify_one() after it. The
  // thread that wakes needs the mutex, and notifying while still holding it makes
  // it wake just to block.
  //
  // The stub drops the item, which is the safe direction to be wrong in: the
  // suite sees an empty queue and says so, and nothing waits for ever.
  void push(const T& item) { (void)item; }

  // TODO 2: take the oldest item, waiting up to timeout for one to arrive.
  //
  //   - Work out a deadline once, at the top: steady_clock::now() + timeout.
  //   - Lock with a std::unique_lock, because waiting has to release the mutex.
  //   - Then a LOOP, not an if:
  //
  //       while (items_.empty() && !closed_) {
  //         ++waits_;
  //         if (ready_.wait_until(lock, deadline) == std::cv_status::timeout) break;
  //       }
  //
  //     A wakeup means look again. The answer may still be no, because another
  //     consumer took the item, or because the wait returned for no reason.
  //   - After the loop, whichever way it ended, if an item is there hand it out and
  //     return kGot. An item that arrived at the instant of the timeout counts.
  //   - Otherwise return kClosed if the queue is closed, or kTimeout if it is not.
  //
  // The stub returns kTimeout at once without waiting.
  Result pop_for(T& out, std::chrono::milliseconds timeout) {
    (void)out;
    (void)timeout;
    return Result::kTimeout;
  }

  // TODO 3: mark the queue finished and wake EVERY waiter.
  //
  // Set closed_ under the lock, then ready_.notify_all(). Not notify_one: each
  // waiting thread has to notice, and one notification leaves the others asleep
  // for ever, which keeps the program from exiting.
  void close() {}

  // Never waits.
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

  // How many times a consumer has gone to sleep. pop_for must count one each time
  // it waits, which is how the suite knows a thread is really asleep.
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
