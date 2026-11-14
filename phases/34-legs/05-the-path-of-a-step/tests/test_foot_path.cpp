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
    {"len", 0.1},
    {"h", 0.05},
    {"t_sw", 0.25},
    {"v_lo", 1.3406649416466985e-07},
    {"v_td", 1.3406649416466985e-07},
    {"v_naive", 0.8},
    {"v_peak", 0.8670912384177565},
    {"a_peak", 19.199520004065107},
    {"ob_h", 0.04},
    {"clear_mm", 9.999999999999988},
    {"rate_slow", 4.717744107589361},
    {"t_fast", 0.125},
    {"rate_fast", 9.435488215178722},
    {"servo_rate", 8.0},
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


RC_TEST("the swing starts and ends at rest, goes from the leaving place to the landing place, and tops out at the step's height") {
  const double p0[3] = {-0.05, 0.08, -0.30}, p1[3] = {0.05, 0.08, -0.30};
  const double h = 0.05, t_sw = 0.25;
  double a[3], b[3], mid[3];
  swing_position(p0, p1, h, 0.0, a);
  swing_position(p0, p1, h, 1.0, b);
  swing_position(p0, p1, h, 0.5, mid);
  for (int k = 0; k < 3; ++k) {
    RC_CHECK_NEAR(a[k], p0[k], 1e-12);
    RC_CHECK_NEAR(b[k], p1[k], 1e-12);
  }
  RC_CHECK_NEAR(mid[2], p0[2] + h, 1e-12);
  RC_CHECK_NEAR(mid[0], 0.0, 1e-12);
  // the speed at both ends is nearly zero
  double e[3], f[3];
  const double d = 1e-4;
  swing_position(p0, p1, h, d, e);
  swing_position(p0, p1, h, 1.0 - d, f);
  const double v_lo = std::sqrt(std::pow(e[0] - a[0], 2) + std::pow(e[1] - a[1], 2) + std::pow(e[2] - a[2], 2)) / (d * t_sw);
  const double v_td = std::sqrt(std::pow(b[0] - f[0], 2) + std::pow(b[1] - f[1], 2) + std::pow(b[2] - f[2], 2)) / (d * t_sw);
  // a path that goes up and down in straight lines at the same height
  const double v_naive = 2.0 * h / (0.5 * t_sw);
  rec("len", 0.10, 2);
  rec("h", h, 2);
  rec("t_sw", t_sw, 2);
  rec("v_lo", v_lo, 3);
  rec("v_td", v_td, 3);
  rec("v_naive", v_naive, 2);
  RC_CHECK(v_lo < 1e-3 && v_td < 1e-3);
  RC_CHECK(v_naive > 0.3);
  // a fraction beyond the range is clamped
  double c[3];
  swing_position(p0, p1, h, 1.7, c);
  RC_CHECK_NEAR(c[0], p1[0], 1e-12);
}

RC_TEST("the peaks of speed and acceleration fall as the swing is given more time, and the stance goes back at the body's speed") {
  const double p0[3] = {-0.05, 0.08, -0.30}, p1[3] = {0.05, 0.08, -0.30};
  double v1, a1, v2, a2;
  swing_peaks(p0, p1, 0.05, 0.25, &v1, &a1);
  swing_peaks(p0, p1, 0.05, 0.50, &v2, &a2);
  rec("v_peak", v1, 2);
  rec("a_peak", a1, 1);
  RC_CHECK_NEAR(v2, 0.5 * v1, 0.02 * v1);
  RC_CHECK_NEAR(a2, 0.25 * a1, 0.03 * a1);
  const double td[3] = {0.05, 0.08, -0.30}, v[2] = {0.4, 0.0};
  double s[3];
  stance_position(td, v, 0.25, s);
  RC_CHECK_NEAR(s[0], -0.05, 1e-12);
  RC_CHECK_NEAR(s[2], -0.30, 1e-12);
  // clearance over an obstacle 0.04 m high in the middle of the step
  double mid[3];
  swing_position(p0, p1, 0.05, 0.5, mid);
  rec("ob_h", 0.04, 2);
  rec("clear_mm", 1000.0 * (mid[2] - p0[2] - 0.04), 0);
  RC_CHECK(mid[2] - p0[2] > 0.04);
}

RC_TEST("the servos have to turn fast enough, and a shorter swing asks for more") {
  LegGeom g;
  const double p0[3] = {-0.05, g.l0, -0.30}, p1[3] = {0.05, g.l0, -0.30};
  double slow = 0.0, fast = 0.0;
  RC_CHECK(swing_peak_joint_rate(g, p0, p1, 0.05, 0.25, -1, &slow));
  RC_CHECK(swing_peak_joint_rate(g, p0, p1, 0.05, 0.125, -1, &fast));
  rec("rate_slow", slow, 1);
  rec("t_fast", 0.125, 3);
  rec("rate_fast", fast, 1);
  rec("servo_rate", 8.0, 0);
  RC_CHECK_NEAR(fast, 2.0 * slow, 0.05 * slow);
  RC_CHECK(slow < 8.0);
  double pk = 0.0;
  const double far_p1[3] = {0.5, g.l0, -0.30};
  RC_CHECK(!swing_peak_joint_rate(g, p0, far_p1, 0.05, 0.25, -1, &pk));
}
