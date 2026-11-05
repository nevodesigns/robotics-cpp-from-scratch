// rc/rt/level_mutex.hpp
//
// Finding a lock-order deadlock without waiting for it, from lesson 07-09,
// graduated.
//
// Lesson 07-01 said that locking by hand is how a robot deadlocks, and that
// scoped_lock avoids it. It did not show one. A deadlock is two threads each
// holding a lock the other wants, so neither can move:
//
//   thread 1:  lock A, then lock B
//   thread 2:  lock B, then lock A
//
// If thread 1 holds A and thread 2 holds B, each waits for the other for ever.
//
// Measured on this curriculum's machine, gcc 11, real threads. The interleaving
// FORCED, with each thread waiting until the other had taken its first lock, and
// then trying for the second with a 200 ms timeout so the test could end: at least
// one thread gave up every time, and in some runs both did. With the same order in
// both threads it never did, and scoped_lock(a, b) against scoped_lock(b, a) always
// finished.
//
// UNFORCED is the finding that matters. The same code, opposite order, two threads
// in a loop, with different amounts of unrelated work between passes, three runs
// each, deadlocks seen in 4000 passes:
//
//   no work between passes       129      3739      3387
//   10 microseconds               54        28        41
//   100 microseconds               0         0         0
//   1000 microseconds              0         3        16
//
// The same bug showed 3739 times in one run and 0 in the next configuration, then
// came back at a longer gap. A run that does not deadlock proves nothing. Lesson
// 07-01 measured the same of a torn read, and a deadlock is worse, because it does
// not corrupt a number, it stops the robot.
//
// So do not wait for the hang. A deadlock needs a cycle of lock orders: one thread
// takes A then B, another takes B then A. That is visible in the ORDER, on one
// thread, whether or not the threads ever overlapped. Give every mutex a level, and
// refuse, and count, any lock taken at a level no higher than one already held.
// Then A then B is always allowed and B then A is always caught, on the first time
// it is attempted, with nothing deadlocked and no timing involved. This is what the
// Linux kernel's lock dependency checker does, at a larger scale.
//
// The check refuses the same level twice as well. Locking a mutex you already hold
// is a deadlock of one thread with itself, and it is the same rule.
//
// What this does not do: it cannot see a lock order that goes through a callback or
// a condition variable it does not know about. It finds the cycles that are made of
// these mutexes, and it finds them early.
//
// The rule that falls out: give locks a fixed order and write it down; take several
// at once with scoped_lock, which orders them for you; hold a lock for as short a time
// as you can and call nothing unknown while you do; and check the order with a
// level, so the first wrong attempt is a report and not a hang.

#ifndef RC_RT_LEVEL_MUTEX_HPP
#define RC_RT_LEVEL_MUTEX_HPP

#include <algorithm>
#include <atomic>
#include <mutex>
#include <vector>

namespace rc {
namespace rt {

// How many times a lock was refused because of its level, across all threads.
inline std::atomic<int>& order_violations() {
  static std::atomic<int> count{0};
  return count;
}

// The levels this thread holds, in the order it took them. thread_local gives each
// thread its own copy, so no thread's list is touched by another and none needs a
// lock of its own.
inline std::vector<int>& held_levels() {
  thread_local std::vector<int> held;
  return held;
}

// A mutex with a level. Locks must be taken in increasing level on any one thread.
class LevelMutex {
 public:
  explicit LevelMutex(int level) : level_(level) {}

  LevelMutex(const LevelMutex&) = delete;
  LevelMutex& operator=(const LevelMutex&) = delete;

  int level() const { return level_; }

  // Takes the lock if that keeps this thread's levels strictly increasing, and says
  // whether it did. A refusal counts a violation and takes nothing, so it cannot
  // block and cannot leave anything held.
  bool lock() {
    std::vector<int>& held = held_levels();
    if (!held.empty() && level_ <= held.back()) {
      ++order_violations();
      return false;
    }
    mutex_.lock();
    held.push_back(level_);
    return true;
  }

  // Releases the lock. Not necessarily the most recent one: unlocking in a
  // different order from locking is allowed, so the level is removed wherever it is.
  void unlock() {
    std::vector<int>& held = held_levels();
    const std::vector<int>::iterator found = std::find(held.begin(), held.end(), level_);
    if (found != held.end()) held.erase(found);
    mutex_.unlock();
  }

 private:
  std::mutex mutex_;
  int level_;
};

// Takes a LevelMutex for the length of a scope, if the order allows it.
//
// Lesson 02-03's guard again, with one addition: ok() says whether the lock was
// taken, because here the answer can be no. A guard that was refused releases
// nothing.
class OrderedLock {
 public:
  explicit OrderedLock(LevelMutex& mutex) : mutex_(mutex), ok_(mutex.lock()) {}

  ~OrderedLock() {
    if (ok_) mutex_.unlock();
  }

  OrderedLock(const OrderedLock&) = delete;
  OrderedLock& operator=(const OrderedLock&) = delete;

  bool ok() const { return ok_; }

 private:
  LevelMutex& mutex_;
  bool ok_;
};

}  // namespace rt
}  // namespace rc

#endif  // RC_RT_LEVEL_MUTEX_HPP
