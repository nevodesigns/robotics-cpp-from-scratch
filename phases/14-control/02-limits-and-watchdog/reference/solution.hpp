#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

class RateLimiter {
 public:
  RateLimiter(double max_rate, double initial = 0.0)
      : max_rate_(max_rate), current_(initial) {}

  double apply(double target, double dt) {
    // A non positive time step carries no information about how much time has
    // passed, so nothing is allowed to change.
    if (dt <= 0.0) return current_;

    const double max_step = max_rate_ * dt;
    const double change = target - current_;

    if (change > max_step) {
      current_ += max_step;
    } else if (change < -max_step) {
      current_ -= max_step;
    } else {
      // Taking the whole remaining change is what makes the value settle. Always
      // moving by max_step would step past the target and back, forever.
      current_ = target;
    }
    return current_;
  }

  void reset(double value) { current_ = value; }
  double current() const { return current_; }

 private:
  double max_rate_ = 1.0;
  double current_ = 0.0;
};

class Watchdog {
 public:
  Watchdog(double timeout, double safe_value)
      : timeout_(timeout), safe_value_(safe_value) {}

  void feed(double now) {
    last_fed_ = now;
    ever_fed_ = true;
  }

  bool expired(double now) const {
    // Never fed means expired. Starting in the trusting state would hide the
    // worst case of all, which is commands that never arrived at any point.
    if (!ever_fed_) return true;
    return (now - last_fed_) > timeout_;
  }

  double guard(double command, double now) const {
    return expired(now) ? safe_value_ : command;
  }

  double safeValue() const { return safe_value_; }

 private:
  double timeout_ = 0.5;
  double safe_value_ = 0.0;
  double last_fed_ = 0.0;
  bool ever_fed_ = false;
};

#endif  // LESSON_SOLUTION_HPP
