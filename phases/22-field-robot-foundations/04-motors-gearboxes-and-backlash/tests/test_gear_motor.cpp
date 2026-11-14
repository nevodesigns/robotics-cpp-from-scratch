#include <rc/test/rc_test.hpp>
#include <sstream>
#include <cstring>

#include <cmath>
#include <iostream>

#include "solution.hpp"
#include <iomanip>
#include <iostream>

// The numbers this lesson publishes: its page, its header and its quiz quote them, so each is held here as it was
// measured. A change that moves one by more than half a unit of its last printed digit fails the suite.
struct Pin {
  const char* key;
  double value;
};
static const Pin kPins[] = {
    {"rpm_nl", 349.99826629398115},
    {"rpm_half", 174.9991331469906},
    {"tau_formula", 0.24609142453120042},
    {"tau_sim", 0.2460000000000002},
    {"stall_weak", 0.31197588293189904},
    {"stall_full", 0.7000000000000001},
    {"lost", 2.023874986950114},
    {"below", -1.000000000000485},
    {"above", 1.000000000000485},
    {"spread", 2.00000000000097},
};

static void rec(const char* key, double value, int digits = 3) {
  std::cout << "@" << key << " " << std::fixed << std::setprecision(digits) << value << std::endl;
  std::cout.unsetf(std::ios::floatfield);
  for (const Pin& pin : kPins) {
    if (std::strcmp(pin.key, key) != 0) continue;
    double allowed = 0.5;
    for (int i = 0; i < digits; ++i) allowed /= 10.0;
    if (!(std::fabs(value - pin.value) <= allowed)) {
      std::ostringstream message;
      message << "the published number " << key << " was " << pin.value << " and is now " << value;
      rc::test::record(__FILE__, __LINE__, message.str());
    }
  }
}


namespace {

const double kPi = 3.14159265358979323846;
const double kRpm = 2.0 * kPi / 60.0;

// A PD controller on the angle the motor's own encoder sees (the gear side), run for a time, towards a target in radians. The damping is enough that the approach does not overshoot, so the output comes to rest against the near edge of the gap.
void go_to(GearMotor& m, double target, double seconds) {
  const int steps = static_cast<int>(seconds / 0.001);
  for (int i = 0; i < steps; ++i) {
    const double volts = 40.0 * (target - m.gear_angle()) - 4.0 * m.output_speed();
    m.step(volts, 0.0, 0.001);
  }
}

}  // namespace

RC_TEST("the data sheet numbers come back out: the no load speed, and half the speed at half the stall torque") {
  GearMotor m;
  for (int i = 0; i < 3000; ++i) m.step(12.0, 0.0, 0.001);
  const double rpm = m.output_speed() / kRpm;
  rec("rpm_nl", rpm, 1);
  RC_CHECK_NEAR(rpm, 350.0, 3.5);

  GearMotor loaded;
  for (int i = 0; i < 3000; ++i) loaded.step(12.0, 0.35, 0.001);
  const double half = loaded.output_speed() / kRpm;
  rec("rpm_half", half, 1);
  RC_CHECK_NEAR(half, 175.0, 5.0);
}

RC_TEST("the time constant from the constants is the time a step takes to reach 63 percent") {
  GearMotor m;
  const double tau = m.time_constant();
  GearMotor settled;
  for (int i = 0; i < 5000; ++i) settled.step(12.0, 0.0, 0.001);
  const double final_speed = settled.output_speed();
  double t = 0.0;
  while (m.output_speed() < 0.632 * final_speed && t < 5.0) {
    m.step(12.0, 0.0, 0.0005);
    t += 0.0005;
  }
  rec("tau_formula", tau);
  rec("tau_sim", t);
  RC_CHECK_NEAR(t, tau, 0.03 * tau);
}

RC_TEST("the current limit caps the torque, and a stronger limit does not help past stall") {
  GearSpec weak;
  weak.max_current = 1.0;
  GearMotor a(weak);
  GearMotor b;
  rec("stall_weak", a.max_torque_out(12.0), 3);
  rec("stall_full", b.max_torque_out(12.0), 3);
  RC_CHECK(a.max_torque_out(12.0) < 0.5 * b.max_torque_out(12.0));
  RC_CHECK_NEAR(b.max_torque_out(12.0), 0.7, 0.01);
}

RC_TEST("on reversing, the gear side crosses the whole gap before the output moves") {
  GearMotor m;
  for (int i = 0; i < 1000; ++i) m.step(5.0, 0.0, 0.001);
  for (int i = 0; i < 3000; ++i) m.step(0.0, 0.0, 0.001);  // let it come to rest, still pressed against the forward side of the gap
  const double start_gear = m.gear_angle();
  const double start_out = m.output_angle();
  RC_CHECK_NEAR(start_gear - start_out, 1.0 * kPi / 180.0, 1e-6);
  int guard = 0;
  while (std::fabs(m.output_angle() - start_out) < 1e-9 && guard++ < 20000) m.step(-5.0, 0.0, 0.001);
  const double lost = (start_gear - m.gear_angle()) * 180.0 / kPi;
  rec("lost", lost, 2);
  RC_CHECK_NEAR(lost, 2.0, 0.05);
}

RC_TEST("a position loop on the motor encoder settles where the encoder says, and the output is off by half the gap, on the side it came from") {
  GearMotor up;
  go_to(up, 90.0 * kPi / 180.0, 3.0);
  const double from_below = (up.output_angle() - 90.0 * kPi / 180.0) * 180.0 / kPi;

  GearMotor down;
  go_to(down, 100.0 * kPi / 180.0, 3.0);
  go_to(down, 90.0 * kPi / 180.0, 3.0);
  const double from_above = (down.output_angle() - 90.0 * kPi / 180.0) * 180.0 / kPi;

  rec("below", from_below, 2);
  rec("above", from_above, 2);
  rec("spread", from_above - from_below, 2);
  RC_CHECK_NEAR(up.gear_angle(), 90.0 * kPi / 180.0, 1e-3);
  RC_CHECK_NEAR(from_below, -1.0, 0.05);
  RC_CHECK_NEAR(from_above, 1.0, 0.05);
}
