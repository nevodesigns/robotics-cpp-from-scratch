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
//
// The outer loop looks at where the joint is and decides how fast it should be going: a
// gain times the error, and never more than a speed limit. The inner loop looks at how
// fast it is going and decides what torque to apply to get there: a proportional and
// integral controller on the speed. The outer loop's output is the inner loop's setpoint.
//
// Three things come of it that a single position loop does not give. The speed limit is
// built in, because the outer loop's answer is clamped before it is ever used. The inner
// loop is fast, so a load that arrives is corrected at the speed loop's rate and not the
// position loop's. And the integral lives in the speed loop, where it removes a steady load
// without being wound up by a large move of position.
class CascadeController {
 public:
  explicit CascadeController(const CascadeGains& gains) : gains_(gains) {}

  // Torque to apply now, given where the joint should be, where it is and how fast it is
  // going.
  double update(double target, double position, double velocity, double dt) {
    double wanted = gains_.position_gain * (target - position);
    if (wanted > gains_.speed_limit) wanted = gains_.speed_limit;
    if (wanted < -gains_.speed_limit) wanted = -gains_.speed_limit;
    speed_command_ = wanted;

    const double speed_error = wanted - velocity;
    const double raw = gains_.speed_kp * speed_error + gains_.speed_ki * integral_;
    double torque = raw;
    if (torque > gains_.torque_limit) torque = gains_.torque_limit;
    if (torque < -gains_.torque_limit) torque = -gains_.torque_limit;

    // Conditional integration. If the motor is already saturated and the error is still
    // pushing the same way, the integral would only grow without changing anything, and
    // then have to be unwound before the loop could respond: the windup of lesson 14-01.
    // So it is held. It is allowed to move when the error is pulling it back out.
    const bool saturated = torque != raw;
    const bool pushing_further = saturated && ((raw > 0.0) == (speed_error > 0.0));
    if (!pushing_further) integral_ += speed_error * dt;

    return torque;
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
