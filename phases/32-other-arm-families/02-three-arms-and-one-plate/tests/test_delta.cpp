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


RC_TEST("the platform is put back where it was asked, from the angles the inverse kinematics gave") {
  Delta d;
  int n = 0, ok = 0;
  double worst = 0.0;
  for (double x = -0.2; x <= 0.2 + 1e-9; x += 0.05)
    for (double y = -0.2; y <= 0.2 + 1e-9; y += 0.05)
      for (double z = -0.55; z <= -0.30 + 1e-9; z += 0.05) {
        ++n;
        double th[3], p[3];
        if (!delta_ik(d, x, y, z, th)) continue;
        RC_CHECK(delta_fk(d, th, p));
        ++ok;
        worst = std::fmax(worst, std::sqrt((p[0] - x) * (p[0] - x) + (p[1] - y) * (p[1] - y) + (p[2] - z) * (p[2] - z)));
      }
  rec("rf", d.rf, 2);
  rec("re", d.re, 2);
  rec("rb", d.rb, 2);
  rec("n_pts", n, 0);
  rec("n_ok", ok, 0);
  rec("err", worst, 9);
  RC_CHECK(ok > 50);
  RC_CHECK(ok < n);
  RC_CHECK(worst < 1e-9);
}

RC_TEST("the three arms are symmetric: a point on the axis gives equal angles, and the reach is a dome") {
  Delta d;
  double th[3];
  RC_CHECK(delta_ik(d, 0.0, 0.0, -0.40, th));
  RC_CHECK_NEAR(th[0], th[1], 1e-9);
  RC_CHECK_NEAR(th[1], th[2], 1e-9);
  // how far out the platform can go at a height, by stepping out until it cannot
  double r = 0.0;
  const double z = -0.40;
  while (delta_ik(d, r + 0.005, 0.0, z, th)) r += 0.005;
  rec("z_test", z, 2);
  rec("r_max", r, 3);
  RC_CHECK(r > 0.1 && r < 0.4);
  double r_low = 0.0;
  while (delta_ik(d, r_low + 0.005, 0.0, -0.58, th)) r_low += 0.005;
  RC_CHECK(r_low < r);  // the reach narrows toward the bottom
  RC_CHECK(!delta_ik(d, 0.0, 0.0, 0.1, th));  // above the base: not reachable
}

RC_TEST("the moving parts of a delta are much lighter than those of a six axis arm") {
  // typical masses: the platform, half of each upper arm (the rest turns on the motor) and the lower rods
  const double platform = 0.20, upper = 0.15, lower = 0.06;
  const double m_delta = platform + 3.0 * (0.5 * upper + 2.0 * lower);
  const double m_six = 18.0;  // a six axis arm of a similar reach moves nearly all of its own links (typical)
  rec("m_delta", m_delta, 2);
  rec("m_six", m_six, 0);
  RC_CHECK(m_delta < 0.1 * m_six);
}
