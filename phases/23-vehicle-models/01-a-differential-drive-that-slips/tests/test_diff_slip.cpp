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
    {"dist_slip", 9.500303999997884},
    {"turn_actual", 72.0379835817986},
    {"more_travel", 25.0},
    {"closure", 4.639696570517785},
    {"ideal_closure", 0.023429451336518383},
    {"eff_read", 0.8000000000000063},
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

// Drives a model with constant wheel speeds for a time.
void drive(DiffSlip& m, double wl, double wr, double seconds, double dt = 0.001) {
  const int n = static_cast<int>(seconds / dt + 0.5);
  for (int i = 0; i < n; ++i) m.step(wl, wr, dt);
}

}  // namespace

RC_TEST("without losses the model follows the arc the wheel speeds describe") {
  DiffSlip m(0.033, 0.160, 0.0, 1.0);
  // Wheels at 20 and 30 rad/s: v = 0.8250 m/s, w = 0.033 * 10 / 0.16 = 2.0625 rad/s, a radius of 0.4 m.
  const double w = 0.033 * 10.0 / 0.160;
  const double t = (kPi / 2.0) / w;
  drive(m, 20.0, 30.0, t);
  const Pose2 p = m.pose();
  RC_CHECK_NEAR(p.theta, kPi / 2.0, 1e-3);
  RC_CHECK_NEAR(p.x, 0.4, 2e-3);
  RC_CHECK_NEAR(p.y, 0.4, 2e-3);
}

RC_TEST("slip takes a fraction of every metre of wheel travel") {
  DiffSlip clean(0.033, 0.160, 0.0, 1.0);
  DiffSlip slipping(0.033, 0.160, 0.05, 1.0);
  const double wheel_speed = 20.0;  // rad/s: 0.66 m/s of wheel
  const double seconds = 10.0 / (0.033 * wheel_speed);
  drive(clean, wheel_speed, wheel_speed, seconds);
  drive(slipping, wheel_speed, wheel_speed, seconds);
  rec("dist_slip", slipping.pose().x, 2);
  // The time is cut into steps of a millisecond, so the distance is right to a fraction of a millimetre a metre.
  RC_CHECK_NEAR(clean.pose().x, 10.0, 1e-3);
  RC_CHECK_NEAR(slipping.pose().x, 9.5, 1e-3);
}

RC_TEST("a turn efficiency of 0.8 turns 72 degrees where the wheels say 90, and costs a quarter more wheel travel to make up") {
  DiffSlip m(0.033, 0.160, 0.0, 0.8);
  // Wheels in opposite directions at 10 rad/s for the time that would be a quarter turn with no loss.
  const double ideal_rate = 0.033 * 20.0 / 0.160;
  const double t = (kPi / 2.0) / ideal_rate;
  drive(m, -10.0, 10.0, t);
  const double turned = m.pose().theta * 180.0 / kPi;
  rec("turn_actual", turned, 1);
  RC_CHECK_NEAR(turned, 72.0, 0.1);
  const double extra = (1.0 / 0.8 - 1.0) * 100.0;
  rec("more_travel", extra, 0);
  DiffSlip longer(0.033, 0.160, 0.0, 0.8);
  drive(longer, -10.0, 10.0, t / 0.8);
  RC_CHECK_NEAR(longer.pose().theta, kPi / 2.0, 1e-3);
}

RC_TEST("a square driven on the commands of the ideal model does not close, and the error is the losses") {
  // Ten metres, a quarter turn, four times, with commands worked out for a model with no loss.
  auto square = [](DiffSlip& m) {
    for (int side = 0; side < 4; ++side) {
      drive(m, 20.0, 20.0, 10.0 / (0.033 * 20.0));
      const double rate = 0.033 * 20.0 / 0.160;
      drive(m, -10.0, 10.0, (kPi / 2.0) / rate);
    }
  };
  DiffSlip ideal(0.033, 0.160, 0.0, 1.0);
  DiffSlip real(0.033, 0.160, 0.02, 0.9);
  square(ideal);
  square(real);
  const double ideal_gap = std::hypot(ideal.pose().x, ideal.pose().y);
  const double gap = std::hypot(real.pose().x, real.pose().y);
  rec("closure", gap, 2);
  std::cout << "  the ideal square closes within " << ideal_gap << " m; with 2 percent slip and an efficiency of 0.9 it is " << gap << " m from the start\n";
  rec("ideal_closure", ideal_gap, 3);
  RC_CHECK(ideal_gap < 0.05);
  RC_CHECK(gap > 3.0);
}

RC_TEST("the efficiency is read back from a measured turn") {
  DiffSlip m(0.033, 0.160, 0.0, 0.8);
  m.reset(Pose2());
  const double travel = 0.033 * 10.0 * 0.5;  // metres of wheel in half a second at 10 rad/s
  drive(m, -10.0, 10.0, 0.5);
  const double eff = turn_efficiency_from(-travel, travel, 0.160, m.pose().theta);
  rec("eff_read", eff, 3);
  RC_CHECK_NEAR(eff, 0.8, 1e-9);
  RC_CHECK(turn_efficiency_from(1.0, 1.0, 0.16, 0.3) == 0.0);
}
