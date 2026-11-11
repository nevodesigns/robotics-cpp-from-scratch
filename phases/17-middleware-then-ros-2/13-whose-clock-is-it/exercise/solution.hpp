#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cmath>

// A clock that does not tick by itself. Its time is whatever the last clock message said, so a simulator can stop it, run it fast or slow,
// or set it somewhere else entirely.
class SimClock {
 public:
  // A clock message: the time is now `t` seconds. A time earlier than the one held is a jump back, such as a simulation restarting.
  void on_clock(double t) {
    // TODO: if t is earlier than the time held (now_): add one to jumps_back_ and to epoch_. Then now_ = t
    (void)t;
    (void)jumps_back_;
    (void)epoch_;
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
    // TODO: The first poll, and any poll after the clock's epoch changed (a jump back), arms the timer: armed_ = true, remember the epoch, next_ = now + period_; fire nothing.
    // Otherwise, if the clock's time is below next_ fire nothing. Else due = floor((now - next_) / period_) + 1 periods have passed.
    // Fire min(due, max_catch_up_) times, add the rest (due minus those fired) to skipped_, and move next_ on by due * period_. Return the number fired.
    (void)clock;
    (void)armed_;
    (void)epoch_;
    (void)next_;
    (void)period_;
    (void)max_catch_up_;
    (void)skipped_;
    return 0;
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
