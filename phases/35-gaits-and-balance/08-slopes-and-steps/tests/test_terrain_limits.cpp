#include <rc/test/rc_test.hpp>
#include <sstream>
#include <cstring>

#include <cmath>

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
    {"h", 0.27},
    {"spare_up", 0.1},
    {"spare_dn", 0.12},
    {"a_fric", 30.96375653207352},
    {"mu", 0.6},
    {"a_wet", 16.69924423399362},
    {"mu_wet", 0.3},
    {"a_tip", 35.134193056915635},
    {"a_level", 27.758540601060023},
    {"first", 2.0},
    {"x_land", 0.1},
    {"s_geo", 0.22879050915251312},
    {"s_under", 0.16765049197767334},
    {"t_sw", 0.2},
    {"servo_rate", 8.0},
    {"s_servo", 0.03325032343282146},
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

const double kDeg = 180.0 / 3.14159265358979323846;

}  // namespace

RC_TEST("the three slope limits follow their formulas, and the first to give is the lowest") {
  const double hw = 0.19, h = 0.27, spare_up = 0.10, spare_dn = 0.12;
  const double mu = 0.6, mu_wet = 0.3;
  const double a_fric = max_slope_friction(mu) * kDeg, a_wet = max_slope_friction(mu_wet) * kDeg;
  const double a_tip = max_slope_tip(hw, h) * kDeg, a_level = max_slope_level(hw, spare_up, spare_dn) * kDeg;
  rec("h", h, 2);
  rec("spare_up", spare_up, 2);
  rec("spare_dn", spare_dn, 2);
  rec("a_fric", a_fric, 1);
  rec("mu", mu, 1);
  rec("a_wet", a_wet, 1);
  rec("mu_wet", mu_wet, 1);
  rec("a_tip", a_tip, 1);
  rec("a_level", a_level, 1);
  const double lim[3] = {a_fric, a_tip, a_level};
  int first = 0;
  for (int i = 1; i < 3; ++i)
    if (lim[i] < lim[first]) first = i;
  rec("first", first, 0);
  RC_CHECK_NEAR(a_fric, std::atan(0.6) * kDeg, 1e-9);
  RC_CHECK_NEAR(a_tip, std::atan(hw / h) * kDeg, 1e-9);
  RC_CHECK_NEAR(a_level, std::atan(spare_up / hw) * kDeg, 1e-9);
  RC_CHECK(a_wet < a_fric);
  RC_CHECK(first == 2 || first == 0);
}

RC_TEST("the highest step by the legs' geometry is greater the further ahead the foot is put (the knee folds less), and a tighter knee limit lowers it") {
  LegGeom g;
  const double h = 0.28;
  const double s0 = max_step_height(g, 0.0, h, -2.6, -0.1), s5 = max_step_height(g, 0.05, h, -2.6, -0.1), s10 = max_step_height(g, 0.10, h, -2.6, -0.1);
  const double tight = max_step_height(g, 0.10, h, -2.2, -0.1);
  rec("x_land", 0.10, 2);
  rec("s_geo", s10, 3);
  rec("s_under", s0, 3);
  RC_CHECK(s5 > 0.05 && s5 < h);
  RC_CHECK(tight < s10);
  RC_CHECK(s0 < s5 && s5 < s10);
  // the step it gives is reachable and one a little higher is not
  double q[3];
  RC_CHECK(leg_ik(g, 0.10, g.l0, -(h - s10 + 1e-6), -1, q));
  RC_CHECK(!(leg_ik(g, 0.10, g.l0, -(h - s10 - 0.01), -1, q) && q[2] >= -2.6));
}

RC_TEST("the servos lower the step that can be swung, and a longer swing allows a higher one") {
  LegGeom g;
  const double h = 0.28, clear = 0.03, rate = 8.0;
  const double geo = max_step_height(g, 0.10, h, -2.6, -0.1);
  const double fast = max_step_servo(g, 0.10, h, 0.20, clear, rate), slow = max_step_servo(g, 0.10, h, 0.35, clear, rate);
  rec("t_sw", 0.20, 2);
  rec("servo_rate", rate, 0);
  rec("s_servo", fast, 3);
  RC_CHECK(fast < geo);
  RC_CHECK(slow > fast);
  RC_CHECK(fast >= 0.0);
}
