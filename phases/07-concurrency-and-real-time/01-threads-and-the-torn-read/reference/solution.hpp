#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <atomic>
#include <mutex>

// Three doubles. Writing one is three separate stores, and a reader arriving
// between any two of them sees a value the robot was never at.
struct Reading {
  double x = 0.0;
  double y = 0.0;
  double theta = 0.0;
};

// One reading shared between a thread that produces and a thread that consumes.
// This is the ordinary shape of sensor sharing in a robot: the consumer wants
// the newest value and does not care about the ones it missed.
class LatestReading {
 public:
  void publish(const Reading& reading) {
    // lock_guard rather than lock and unlock by hand. It releases in its
    // destructor, so the mutex is freed on an early return and while an
    // exception unwinds. Unlocking by hand is how a robot deadlocks.
    const std::lock_guard<std::mutex> held(mutex_);
    latest_ = reading;
    ++count_;
  }

  Reading latest() const {
    // The read is guarded too. A mutex only helps when every access takes it:
    // one unguarded reader is enough to bring the tearing back.
    const std::lock_guard<std::mutex> held(mutex_);
    return latest_;
  }

  long count() const {
    const std::lock_guard<std::mutex> held(mutex_);
    return count_;
  }

 private:
  // Mutable so that latest() and count() can stay const. Taking a lock does not
  // change what the object means, which is the case mutable exists for.
  mutable std::mutex mutex_;
  Reading latest_;
  long count_ = 0;
};

// A single value that fits in a machine word needs no lock. This is exactly
// right for telling a thread to stop, and exactly wrong for a pose, because
// three doubles do not fit in a word.
class StopFlag {
 public:
  void request_stop() { stop_.store(true); }
  bool stopped() const { return stop_.load(); }

 private:
  std::atomic<bool> stop_{false};
};

#endif  // LESSON_SOLUTION_HPP
