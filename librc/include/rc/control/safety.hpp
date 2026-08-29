// rc/control/safety.hpp
//
// The rate limiter and watchdog from lesson 14-02, graduated.
//
// Every capstone in this curriculum is required by its own tests to route
// actuator commands through both. The watchdog has no default safe value on
// purpose: zero is safe for a wheel and dangerous for an arm holding a load, and
// a library that chooses silently is making a safety decision on behalf of
// somebody who never knew one was being made.

#ifndef RC_CONTROL_SAFETY_HPP
#define RC_CONTROL_SAFETY_HPP

namespace rc {
namespace control {

class RateLimiter {
 public:
  explicit RateLimiter(double max_rate, double initial = 0.0)
      : max_rate_(max_rate), current_(initial) {}

  double apply(double target, double dt) {
    if (dt <= 0.0) return current_;

    const double max_step = max_rate_ * dt;
    const double change = target - current_;

    if (change > max_step) current_ += max_step;
    else if (change < -max_step) current_ -= max_step;
    else current_ = target;   // taking the whole remainder is what makes it settle

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
  Watchdog(double timeout, double safe_value) : timeout_(timeout), safe_value_(safe_value) {}

  void feed(double now) {
    last_fed_ = now;
    ever_fed_ = true;
  }

  // Never fed counts as expired. A watchdog that begins life trusting hides the
  // worst case of all, which is commands that never arrived at any point.
  bool expired(double now) const {
    if (!ever_fed_) return true;
    return (now - last_fed_) > timeout_;
  }

  double guard(double command, double now) const {
    return expired(now) ? safe_value_ : command;
  }

  double safe_value() const { return safe_value_; }

 private:
  double timeout_ = 0.5;
  double safe_value_ = 0.0;
  double last_fed_ = 0.0;
  bool ever_fed_ = false;
};

}  // namespace control
}  // namespace rc

#endif  // RC_CONTROL_SAFETY_HPP
