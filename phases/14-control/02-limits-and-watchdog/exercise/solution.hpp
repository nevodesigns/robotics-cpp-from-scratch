#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

// Limits how fast a command may change.
class RateLimiter {
 public:
  RateLimiter(double max_rate, double initial = 0.0)
      : max_rate_(max_rate), current_(initial) {}

  // Moves current towards target by at most max_rate * dt, and returns it.
  // Takes the whole remaining change when it is smaller than one step, so the
  // value settles instead of stepping past and coming back.
  double apply(double target, double dt) {
    // TODO
    (void)target;
    (void)dt;
    return current_;
  }

  void reset(double value) { current_ = value; }
  double current() const { return current_; }

 private:
  double max_rate_ = 1.0;
  double current_ = 0.0;
};

// Forces the output to a safe value when commands stop arriving.
class Watchdog {
 public:
  // timeout    how long a command stays valid, in seconds
  // safe_value what to output once it has expired. This is a decision about the
  //            machine, which is why it has no default.
  Watchdog(double timeout, double safe_value)
      : timeout_(timeout), safe_value_(safe_value) {}

  // Records that a fresh command arrived at time now.
  void feed(double now) {
    // TODO
    (void)now;
  }

  // True when no command has arrived within the timeout.
  // A watchdog that has never been fed is expired, not fresh.
  bool expired(double now) const {
    // TODO
    (void)now;
    return false;
  }

  // The command when it is fresh, the safe value when it is not.
  double guard(double command, double now) const {
    // TODO
    (void)now;
    return command;
  }

  double safeValue() const { return safe_value_; }

 private:
  double timeout_ = 0.5;
  double safe_value_ = 0.0;
  double last_fed_ = 0.0;
  bool ever_fed_ = false;
};

#endif  // LESSON_SOLUTION_HPP
