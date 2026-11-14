#include <rc/test/rc_test.hpp>
#include <sstream>
#include <cstring>

#include <cmath>
#include <iostream>

#include <rc/field/gear_motor.hpp>
#include <rc/field/terrain.hpp>

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
    {"slip_deg", 50.19442890773481},
    {"r1", 0.625},
    {"r15", 0.9375},
    {"need_gravel", 34.478347194289924},
    {"need_ice", 7.661854932064428},
    {"have", 28.0},
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
[[maybe_unused]] static void recs(const char* key, const std::string& value) { std::cout << "@" << key << " " << value << std::endl; }


namespace {

const double kPi = 3.14159265358979323846;

}  // namespace

RC_TEST("in a pivot the front wheels slide at the angle of the chassis' own shape, whatever the speed") {
  const double a = front_slip_angle(0.30, 0.25) * 180.0 / kPi;
  rec("slip_deg", a, 1);
  RC_CHECK_NEAR(a, std::atan(1.2) * 180.0 / kPi, 1e-9);
  RC_CHECK_NEAR(front_slip_angle(0.25, 0.25) * 180.0 / kPi, 45.0, 1e-9);
  RC_CHECK(front_slip_angle(0.60, 0.25) > front_slip_angle(0.30, 0.25));
}

RC_TEST("a skid steer with an ICR coefficient of 1.4 moves as a differential drive with an efficiency of 1 over 1.4") {
  SkidGeometry g;
  SkidSteer skid(g, 1.4, 0.0);
  DiffSlip diff(g.wheel_radius, g.track, 0.0, 1.0 / 1.4);
  for (int i = 0; i < 4000; ++i) {
    const double wl = 8.0 + 4.0 * std::sin(0.002 * i);
    const double wr = 14.0;
    skid.step(wl, wr, 0.001);
    diff.step(wl, wr, 0.001);
  }
  RC_CHECK_NEAR(skid.pose().x, diff.pose().x, 1e-12);
  RC_CHECK_NEAR(skid.pose().y, diff.pose().y, 1e-12);
  RC_CHECK_NEAR(skid.pose().theta, diff.pose().theta, 1e-12);
  RC_CHECK(std::fabs(skid.pose().theta) > 0.1);
}

RC_TEST("the centre of rotation is as far again as the ICR coefficient says, and a pivot has it at the centre") {
  SkidGeometry g;
  const SkidMotion a = skid_motion(g, 1.0, 20.0, 30.0);
  const SkidMotion b = skid_motion(g, 1.5, 20.0, 30.0);
  rec("r1", a.icr_radius, 3);
  rec("r15", b.icr_radius, 3);
  RC_CHECK_NEAR(b.icr_radius, 1.5 * a.icr_radius, 1e-9);
  RC_CHECK_NEAR(a.v, 0.05 * 25.0, 1e-12);
  RC_CHECK_NEAR(skid_motion(g, 1.0, -10.0, 10.0).icr_radius, 0.0, 1e-12);
  RC_CHECK(skid_motion(g, 1.0, 10.0, 10.0).icr_radius > 1e9);
}

RC_TEST("a pivot needs a force from each side that depends on the floor, and the motors may not have it") {
  SkidGeometry g;
  const double gravel = pivot_thrust_per_side(g.mass, rc::field::gravel().mu, g.wheelbase, g.track);
  const double grass = pivot_thrust_per_side(g.mass, rc::field::grass().mu, g.wheelbase, g.track);
  const double ice = pivot_thrust_per_side(g.mass, rc::field::ice().mu, g.wheelbase, g.track);
  rec("need_gravel", gravel, 1);
  rec("need_ice", ice, 1);
  RC_CHECK_NEAR(gravel, 0.45 * 10.0 * 9.81 * std::sqrt(0.09 + 0.0625) / 0.5, 1e-9);
  RC_CHECK(ice < grass && grass < gravel);

  // Two gear motors a side, at the rim of a wheel of 0.05 m.
  rc::field::GearMotor motor;
  const double have = 2.0 * motor.max_torque_out(12.0) / g.wheel_radius;
  rec("have", have, 1);
  const bool gravel_ok = can_pivot(have, g.mass, rc::field::gravel().mu, g.wheelbase, g.track);
  const bool grass_ok = can_pivot(have, g.mass, rc::field::grass().mu, g.wheelbase, g.track);
  const bool ice_ok = can_pivot(have, g.mass, rc::field::ice().mu, g.wheelbase, g.track);
  recs("verdict", gravel_ok ? "can pivot" : "cannot pivot");
  std::cout << "  can it pivot on gravel: " << gravel_ok << ", on grass: " << grass_ok << ", on ice: " << ice_ok << "\n";
  RC_CHECK(ice_ok);
  RC_CHECK(grass_ok);
  RC_CHECK(!gravel_ok);
  RC_CHECK_NEAR(have, 28.0, 0.5);
}
