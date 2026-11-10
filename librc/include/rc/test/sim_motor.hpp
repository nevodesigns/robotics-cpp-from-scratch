// rc/test/sim_motor.hpp
//
// A motor port and a fake with the rules of the real one, from lesson 05-10, graduated.
//
// A test that records the calls made to a port and compares them with a list broke for 5 of 5 changes that do not matter (a ramp in 8 steps or 2, a second enable, the final speed set again,
// a disable first), and a test of the fake's state, whether it is enabled, its speed and how many rules were broken, broke for 0. Both caught all 3 bugs, but the state says what the bug did: never enabling gave 4 speeds
// ignored by the driver and a final speed of 0, and a last step too big gave 1 refused and a final speed of 0.75. The fake counts a speed set while disabled and a speed past the limit, and does not obey either.

#ifndef RC_TEST_SIM_MOTOR_HPP
#define RC_TEST_SIM_MOTOR_HPP

#include <cmath>

namespace rc {
namespace test {

// What a motor driver lets a program do. The real one talks to hardware, and a test cannot.
class MotorPort {
 public:
  virtual ~MotorPort() = default;
  virtual void enable() = 0;
  virtual void set_speed(double speed) = 0;      // metres a second, forward positive
  virtual void disable() = 0;
};

// A fake that behaves like the motor: it has a state, and it has the rules the real driver has.
//
// The rules are what a test needs to be able to check. A speed set while the driver is disabled is ignored by the
// real hardware, and a speed past the limit is refused. The fake counts each of those as a violation, so a test can
// ask "did the program ever break a rule?" without caring which calls it made.
class SimPort : public MotorPort {
 public:
  explicit SimPort(double limit) : limit_(limit) {}

  void enable() override { enabled_ = true; }

  void set_speed(double speed) override {
    if (!enabled_) {
      ++violations_;              // the driver would have ignored it
      return;
    }
    if (std::fabs(speed) > limit_) {
      ++violations_;              // the driver would have refused it
      return;
    }
    speed_ = speed;
  }

  void disable() override {
    enabled_ = false;
    speed_ = 0.0;                 // a disabled motor is not turning
  }

  bool enabled() const { return enabled_; }
  double speed() const { return speed_; }
  int violations() const { return violations_; }

 private:
  double limit_;
  bool enabled_ = false;
  double speed_ = 0.0;
  int violations_ = 0;
};

}  // namespace test
}  // namespace rc

#endif  // RC_TEST_SIM_MOTOR_HPP
