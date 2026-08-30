#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <atomic>
#include <mutex>

struct Reading {
  double x = 0.0;
  double y = 0.0;
  double theta = 0.0;
};

class LatestReading {
 public:
  // TODO: publish a reading so that a reader can never see one half written.
  //
  // Use std::lock_guard rather than locking and unlocking by hand, so the mutex
  // is released on every path out including an exception.
  void publish(const Reading& reading) {
    latest_ = reading;
    ++count_;
  }

  // TODO: the read has to take the same lock. A mutex only helps when every
  // access takes it, and one unguarded reader brings the tearing straight back.
  Reading latest() const { return latest_; }

  long count() const { return count_; }

 private:
  // A mutex that latest() and count() can lock while staying const. Taking a
  // lock does not change what the object means, which is what mutable is for.
  mutable std::mutex mutex_;
  Reading latest_;
  long count_ = 0;
};

class StopFlag {
 public:
  // TODO: a plain bool shared between threads is a data race, and the compiler
  // may keep it in a register so the other thread never sees the change. Use a
  // type that is safe to read and write from several threads at once.
  void request_stop() { stop_ = true; }
  bool stopped() const { return stop_; }

 private:
  bool stop_ = false;
};

#endif  // LESSON_SOLUTION_HPP
