#include <rc/test/rc_test.hpp>

#include <cmath>

#include "solution.hpp"

namespace {

// A first order plant: something that approaches the command it is given, at a
// rate set by its time constant. A motor with inertia behaves like this, and it
// is enough to tell a working controller from a broken one.
class Plant {
 public:
  explicit Plant(double time_constant) : time_constant_(time_constant) {}

  double step(double command, double dt) {
    value_ += (command - value_) * (dt / time_constant_);
    return value_;
  }

  double value() const { return value_; }

 private:
  double time_constant_;
  double value_ = 0.0;
};

}  // namespace

RC_TEST("with no error there is no command") {
  Pid pid(1.0, 0.0, 0.0, -1.0, 1.0);
  RC_CHECK_NEAR(pid.update(0.0, 0.0, 0.01), 0.0, 1e-9);
}

RC_TEST("proportional pushes in proportion to the error") {
  Pid pid(2.0, 0.0, 0.0, -10.0, 10.0);
  RC_CHECK_NEAR(pid.update(1.0, 0.0, 0.01), 2.0, 1e-9);
  RC_CHECK_NEAR(pid.update(1.0, 0.5, 0.01), 1.0, 1e-9);
}

RC_TEST("the output never leaves its limits") {
  Pid pid(100.0, 0.0, 0.0, -1.0, 1.0);
  RC_CHECK_NEAR(pid.update(10.0, 0.0, 0.01), 1.0, 1e-9);
  RC_CHECK_NEAR(pid.update(-10.0, 0.0, 0.01), -1.0, 1e-9);
}

RC_TEST("proportional alone leaves a steady state error") {
  Pid pid(0.5, 0.0, 0.0, -1.0, 1.0);
  Plant plant(0.2);
  double measured = 0.0;
  for (int i = 0; i < 2000; ++i) {
    measured = plant.step(pid.update(1.0, measured, 0.01), 0.01);
  }
  // It gets close and stops short. That shortfall is what integral exists for.
  RC_CHECK(measured < 0.9);
  RC_CHECK(measured > 0.1);
}

RC_TEST("adding integral removes the steady state error") {
  Pid pid(0.5, 2.0, 0.0, -1.0, 1.0);
  Plant plant(0.2);
  double measured = 0.0;
  for (int i = 0; i < 4000; ++i) {
    measured = plant.step(pid.update(1.0, measured, 0.01), 0.01);
  }
  RC_CHECK_NEAR(measured, 1.0, 0.02);
}

RC_TEST("anti windup keeps the integral bounded while saturated") {
  // The setpoint is far beyond anything the output limits can deliver, so the
  // error never clears. Without anti windup the integral grows without bound.
  Pid pid(1.0, 5.0, 0.0, -1.0, 1.0);
  for (int i = 0; i < 1000; ++i) pid.update(100.0, 0.0, 0.01);
  RC_CHECK(std::fabs(pid.integral()) < 50.0);
}

RC_TEST("a wound up controller recovers promptly when the setpoint drops") {
  Pid pid(1.0, 5.0, 0.0, -1.0, 1.0);
  for (int i = 0; i < 1000; ++i) pid.update(100.0, 0.0, 0.01);

  // The demand is now satisfied. A controller that wound up would keep
  // commanding full power for a long time. This one should let go quickly.
  double output = 0.0;
  for (int i = 0; i < 200; ++i) output = pid.update(0.0, 0.0, 0.01);
  RC_CHECK(output < 0.5);
}

RC_TEST("changing the setpoint does not kick the output") {
  // Derivative on error would spike enormously on this step. Derivative on
  // measurement does not, because the measurement did not move.
  Pid pid(1.0, 0.0, 10.0, -1000.0, 1000.0);
  pid.update(0.0, 0.0, 0.01);
  const double after_setpoint_jump = pid.update(1.0, 0.0, 0.01);
  RC_CHECK(std::fabs(after_setpoint_jump) < 5.0);
}

RC_TEST("derivative damps a moving measurement") {
  Pid pid(0.0, 0.0, 1.0, -100.0, 100.0);
  pid.update(0.0, 0.0, 0.1);
  // The measurement rose by 1.0 over 0.1 seconds, so the derivative term should
  // push back, which means a negative command.
  RC_CHECK(pid.update(0.0, 1.0, 0.1) < 0.0);
}

RC_TEST("a time step of zero or less holds the previous output") {
  Pid pid(1.0, 0.0, 0.0, -10.0, 10.0);
  const double first = pid.update(1.0, 0.0, 0.01);
  RC_CHECK_NEAR(pid.update(5.0, 0.0, 0.0), first, 1e-9);
  RC_CHECK_NEAR(pid.update(5.0, 0.0, -0.01), first, 1e-9);
}

RC_TEST("reset clears the accumulated state") {
  Pid pid(1.0, 5.0, 0.0, -10.0, 10.0);
  for (int i = 0; i < 100; ++i) pid.update(1.0, 0.0, 0.01);
  RC_CHECK(std::fabs(pid.integral()) > 0.0);

  pid.reset();
  RC_CHECK_NEAR(pid.integral(), 0.0, 1e-9);
  RC_CHECK_NEAR(pid.lastOutput(), 0.0, 1e-9);
}
