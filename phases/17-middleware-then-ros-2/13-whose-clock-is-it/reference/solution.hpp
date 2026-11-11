#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cmath>

// A clock that does not tick by itself. Its time is whatever the last clock message said, so a simulator can stop it, run it fast or slow,
// or set it somewhere else entirely.
class SimClock {
 public:
  // A clock message: the time is now `t` seconds. A time earlier than the one held is a jump back, such as a simulation restarting.
  void on_clock(double t) {
    if (t < now_) {
      ++jumps_back_;
      ++epoch_;
    }
    now_ = t;
  }

  double now() const { return now_; }
  int epoch() const { return epoch_; }          // changes whenever the time jumps back
  int jumps_back() const { return jumps_back_; }

 private:
  double now_ = 0.0;
  int epoch_ = 0;
  int jumps_back_ = 0;
};

// Something that should happen every `period` seconds of the clock's time. Poll it, and it says how many times it fires.
//   - A clock that has not moved fires nothing.
//   - A clock that jumped forward over many periods fires at most `max_catch_up` times, however many were missed; the rest are counted as skipped.
//   - A clock that jumped back starts the timer again from the new time, instead of waiting for the old one to come round.
class SimTimer {
 public:
  SimTimer(double period, int max_catch_up) : period_(period > 0.0 ? period : 1.0), max_catch_up_(max_catch_up < 1 ? 1 : max_catch_up) {}

  int poll(const SimClock& clock) {
    if (!armed_ || clock.epoch() != epoch_) {
      armed_ = true;
      epoch_ = clock.epoch();
      next_ = clock.now() + period_;
      return 0;
    }
    if (clock.now() < next_) return 0;
    const int due = static_cast<int>(std::floor((clock.now() - next_) / period_)) + 1;
    const int fire = due < max_catch_up_ ? due : max_catch_up_;
    skipped_ += due - fire;
    next_ += due * period_;
    return fire;
  }

  int skipped() const { return skipped_; }

 private:
  double period_ = 1.0;
  int max_catch_up_ = 1;
  bool armed_ = false;
  int epoch_ = 0;
  double next_ = 0.0;
  int skipped_ = 0;
};

#endif  // LESSON_SOLUTION_HPP
