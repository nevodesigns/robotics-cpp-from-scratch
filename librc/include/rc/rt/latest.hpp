// rc/rt/latest.hpp
//
// The shared value slot from lesson 07-01, graduated.
//
// The ordinary shape of sensor sharing in a robot: a thread produces readings,
// a thread consumes them, and the consumer wants the newest one and does not
// care about the ones it missed. A queue would be wrong here, because a queue
// preserves history and a stale pose is worse than no pose.
//
// Templated where the lesson was concrete. The lesson used one Reading type
// because meeting templates and memory ordering in the same hour helps nobody.
// A pose, a joint vector and a battery voltage all want this slot, so the
// graduated version takes the payload as a parameter.

#ifndef RC_RT_LATEST_HPP
#define RC_RT_LATEST_HPP

#include <atomic>
#include <mutex>

namespace rc {
namespace rt {

// One value, written by one thread and read by others, without tearing.
//
// A mutex rather than an atomic, because an atomic only covers what fits in a
// machine word and a pose is three doubles. The cost is that a reader can be
// made to wait for however long the writer holds the lock, which is bounded
// here only because publishing is a copy and nothing else. Put anything slower
// inside the lock and this becomes the latency problem lesson 07-02 measured.
template <class T>
class Latest {
 public:
  Latest() = default;
  explicit Latest(const T& initial) : value_(initial) {}

  void publish(const T& value) {
    // lock_guard rather than lock and unlock by hand: it releases in its
    // destructor, so the mutex is freed on an early return and while an
    // exception unwinds.
    const std::lock_guard<std::mutex> held(mutex_);
    value_ = value;
    ++count_;
  }

  // By value, not by reference. Returning a reference would hand the caller a
  // pointer into data the writer is free to change the moment the lock is
  // dropped, which is the tearing this class exists to prevent, reintroduced
  // through the return type.
  T latest() const {
    const std::lock_guard<std::mutex> held(mutex_);
    return value_;
  }

  // How many times anything has been published. A consumer that samples this
  // can tell a fresh reading from the same one seen twice, which a copy of the
  // value alone cannot.
  long count() const {
    const std::lock_guard<std::mutex> held(mutex_);
    return count_;
  }

 private:
  // Mutable so the readers stay const. Taking a lock does not change what the
  // object means, which is the case mutable exists for.
  mutable std::mutex mutex_;
  T value_{};
  long count_ = 0;
};

// A single value that fits in a machine word needs no lock. Exactly right for
// telling a thread to stop, and exactly wrong for a pose.
class StopFlag {
 public:
  void request_stop() { stop_.store(true, std::memory_order_release); }
  bool stopped() const { return stop_.load(std::memory_order_acquire); }
  void reset() { stop_.store(false, std::memory_order_release); }

 private:
  std::atomic<bool> stop_{false};
};

}  // namespace rt
}  // namespace rc

#endif  // RC_RT_LATEST_HPP
