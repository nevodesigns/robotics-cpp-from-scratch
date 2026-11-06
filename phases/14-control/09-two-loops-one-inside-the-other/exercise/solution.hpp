#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cmath>

// How the two loops are set.
struct CascadeGains {
  double position_gain = 10.0;   // speed asked for per radian of error: the outer loop is a plain gain
  double speed_limit = 3.0;      // the fastest the outer loop will ever ask for, radians a second
  double speed_kp = 40.0;        // torque per radian a second of speed error
  double speed_ki = 400.0;       // torque per radian of accumulated speed error
  double torque_limit = 20.0;    // what the motor can supply, newton metres
};

// A position loop wrapped round a speed loop.
class CascadeController {
 public:
  explicit CascadeController(const CascadeGains& gains) : gains_(gains) {}

  // Torque to apply now, given where the joint should be, where it is and how fast it is
  // going.
  double update(double target, double position, double velocity, double dt) {
    // TODO: three steps.
    //
    // Outer loop: the speed wanted is position_gain times (target - position), clamped to plus
    // and minus speed_limit. Keep it in speed_command_.
    //
    // Inner loop: the speed error is the speed wanted minus velocity. The raw torque is
    // speed_kp * speed error + speed_ki * integral_. The torque applied is the raw one clamped
    // to plus and minus torque_limit.
    //
    // The integral: add speed error * dt to integral_, EXCEPT when the motor is saturated and
    // the error is still pushing the same way (the raw torque and the speed error have the
    // same sign). Then it would only grow without changing anything and have to be unwound
    // before the loop could respond. That is conditional integration, and it is the whole of
    // the anti-windup.
    (void)target;
    (void)position;
    (void)velocity;
    (void)dt;
    (void)gains_;
    (void)integral_;
    (void)speed_command_;
    return 0.0;
  }

  void reset() {
    integral_ = 0.0;
    speed_command_ = 0.0;
  }

  // What the outer loop asked of the inner one, which is how a speed limit is seen to hold.
  double speed_command() const { return speed_command_; }
  double integral() const { return integral_; }

 private:
  CascadeGains gains_;
  double integral_ = 0.0;
  double speed_command_ = 0.0;
};

#endif  // LESSON_SOLUTION_HPP
