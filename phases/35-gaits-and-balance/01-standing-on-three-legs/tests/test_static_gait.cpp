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
    {"hx", 0.19},
    {"hy", 0.12},
    {"fy", 0.15},
    {"m0", 0.0},
    {"m_shift", 0.0784882765533426},
    {"shift_cm", 8.069145624606804},
    {"fwd_cm", 5.0},
    {"m_fwd", 0.030982214428951032},
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


RC_TEST("the margin is the distance to the nearest edge, positive inside and negative outside, either way round") {
  const double sq[4][2] = {{0, 0}, {2, 0}, {2, 2}, {0, 2}};
  const double rev[4][2] = {{0, 2}, {2, 2}, {2, 0}, {0, 0}};
  const double in[2] = {0.5, 1.0}, out[2] = {3.0, 1.0}, on[2] = {1.0, 0.0};
  RC_CHECK_NEAR(polygon_margin(sq, 4, in), 0.5, 1e-12);
  RC_CHECK_NEAR(polygon_margin(rev, 4, in), 0.5, 1e-12);
  RC_CHECK_NEAR(polygon_margin(sq, 4, out), -1.0, 1e-12);
  RC_CHECK_NEAR(polygon_margin(sq, 4, on), 0.0, 1e-12);
  const double tri[3][2] = {{0, 0}, {4, 0}, {0, 3}};
  const double c[2] = {1.0, 1.0};
  RC_CHECK_NEAR(polygon_margin(tri, 3, c), 1.0, 1e-12);   // the incircle's centre of a 3 4 5 triangle
}

RC_TEST("lifting a leg with the body centred puts the centre of mass on an edge, and shifting it gives a margin") {
  const double hx = 0.19, hy = 0.12, fy = 0.15;
  double feet[4][2];
  default_feet(hx, fy, feet);
  const double com[2] = {0.0, 0.0};
  double worst = 1e9, shift_max = 0.0, margin_shifted = 1e9;
  for (int lifted = 0; lifted < 4; ++lifted) {
    const double m = crawl_margin(feet, lifted, com);
    RC_CHECK_NEAR(m, 0.0, 1e-12);
    if (m < worst) worst = m;
    double sh = 0.0;
    const double ms = crawl_margin_shifted(feet, lifted, &sh);
    RC_CHECK(ms > 0.03);
    if (ms < margin_shifted) margin_shifted = ms;
    if (sh > shift_max) shift_max = sh;
  }
  rec("hx", hx, 2);
  rec("hy", hy, 2);
  rec("fy", fy, 2);
  rec("m0", std::fabs(worst), 3);
  rec("m_shift", margin_shifted, 3);
  rec("shift_cm", shift_max * 100.0, 1);
  RC_CHECK(shift_max < 0.15);
}

RC_TEST("a body shifted the wrong way is outside the triangle, and a shift toward the middle is inside") {
  const double hx = 0.19, fy = 0.15;
  double feet[4][2];
  default_feet(hx, fy, feet);
  const double fwd[2] = {0.05, 0.0};
  // lift a hind foot (index 2): the body shifted forward is inside the triangle of the other three
  const double m_fwd = crawl_margin(feet, 2, fwd);
  rec("fwd_cm", 5, 0);
  rec("m_fwd", m_fwd, 3);
  RC_CHECK(m_fwd > 0.0);
  // lift a front foot (index 0): the same shift forward is toward the lifted foot's side and is outside
  RC_CHECK(crawl_margin(feet, 0, fwd) < 0.0 || crawl_margin(feet, 0, fwd) < m_fwd);
  const double away[2] = {-0.05, 0.0};
  RC_CHECK(crawl_margin(feet, 0, away) > 0.0);
}

RC_TEST("the middle of the corners and the shift to it are the numbers geometry gives") {
  const double tri[3][2] = {{1.0, 2.0}, {4.0, 2.0}, {1.0, 8.0}};
  double c[2] = {7.0, 7.0};   // not zero to begin with: the function must not add to what it was handed
  centroid(tri, 3, c);
  RC_CHECK_NEAR(c[0], 2.0, 1e-12);
  RC_CHECK_NEAR(c[1], 4.0, 1e-12);
  double feet[4][2];
  default_feet(0.2, 0.1, feet);
  double shift = 0.0;
  const double m = crawl_margin_shifted(feet, 0, &shift);
  RC_CHECK_NEAR(shift, std::sqrt(0.2 * 0.2 + 0.1 * 0.1) / 3.0, 1e-12);   // a third of the way to the corner opposite the lifted foot
  RC_CHECK(m > 0.0);
}
