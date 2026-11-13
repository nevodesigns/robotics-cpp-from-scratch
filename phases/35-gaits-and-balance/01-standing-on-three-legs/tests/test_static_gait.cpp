#include <rc/test/rc_test.hpp>

#include <cmath>

#include "solution.hpp"
#include <iomanip>
#include <iostream>

static void rec(const char* key, double value, int digits = 3) {
  std::cout << "@" << key << " " << std::fixed << std::setprecision(digits) << value << std::endl;
  std::cout.unsetf(std::ios::floatfield);
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
