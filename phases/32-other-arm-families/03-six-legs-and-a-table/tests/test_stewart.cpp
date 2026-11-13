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


namespace {

StewartPose home(double h) {
  StewartPose p;
  p.v[2] = h;
  return p;
}

}  // namespace

RC_TEST("the leg lengths at home are all equal for a symmetric hexapod, and a raise lengthens them") {
  Stewart s;
  double l[6], l2[6];
  const double h = 0.30;
  stewart_lengths(s, home(h), l);
  stewart_lengths(s, home(h + 0.02), l2);
  rec("h", h, 2);
  rec("l_home", l[0], 3);
  for (int i = 1; i < 6; ++i) RC_CHECK_NEAR(l[i], l[0], 1e-9);
  for (int i = 0; i < 6; ++i) RC_CHECK(l2[i] > l[i]);
  // the length for a pose at home, worked out by hand for the first leg
  const double dx = s.px[0] - s.bx[0], dy = s.py[0] - s.by[0];
  RC_CHECK_NEAR(l[0], std::sqrt(dx * dx + dy * dy + h * h), 1e-12);
}

RC_TEST("a raise and a tilt need a range of leg lengths") {
  Stewart s;
  StewartPose p = home(0.30);
  p.v[2] += 0.05;
  p.v[3] = 10.0 * 3.14159265358979323846 / 180.0;
  double l[6];
  stewart_lengths(s, p, l);
  double lo = l[0], hi = l[0];
  for (int i = 1; i < 6; ++i) {
    lo = std::fmin(lo, l[i]);
    hi = std::fmax(hi, l[i]);
  }
  rec("dz_mm", 50, 0);
  rec("tilt_deg", 10, 0);
  rec("l_min", lo, 3);
  rec("l_max", hi, 3);
  RC_CHECK(hi - lo > 0.02);
}

RC_TEST("the pose is found again from the leg lengths, from a guess that is some way off") {
  Stewart s;
  StewartPose truth = home(0.32);
  truth.v[0] = 0.02;
  truth.v[1] = -0.015;
  truth.v[3] = 0.08;
  truth.v[4] = -0.05;
  truth.v[5] = 0.1;
  double len[6];
  stewart_lengths(s, truth, len);
  StewartPose guess = home(0.30);
  guess.v[0] = 0.0;
  guess.v[3] = 0.0;
  int iters = 0;
  RC_CHECK(stewart_pose(s, len, guess, &iters));
  double worst = 0.0;
  for (int i = 0; i < 6; ++i) worst = std::fmax(worst, std::fabs(guess.v[i] - truth.v[i]));
  rec("guess_mm", 20.0 * 1.0 + 0.0, 0);
  rec("guess_deg", 4.6, 1);
  rec("err", worst, 9);
  rec("iters", iters, 0);
  RC_CHECK(worst < 1e-8);
  RC_CHECK(iters >= 1 && iters < 20);
}

RC_TEST("the legs control the platform well at home and lose a direction elsewhere") {
  Stewart s;
  const double cond_home = stewart_condition(s, home(0.30));
  double cond_min = 1.0;
  for (int k = 0; k <= 90; ++k) {
    StewartPose p = home(0.30);
    p.v[5] = k * 3.14159265358979323846 / 180.0;
    cond_min = std::fmin(cond_min, stewart_condition(s, p));
  }
  // a pose with the platform nearly on the base is poor too
  const double cond_flat = stewart_condition(s, home(0.03));
  StewartPose p80 = home(0.30);
  p80.v[5] = 80.0 * 3.14159265358979323846 / 180.0;
  rec("cond_home", cond_home, 3);
  rec("cond_80", stewart_condition(s, p80), 3);
  rec("cond_min", cond_min, 3);
  RC_CHECK(stewart_condition(s, p80) < 0.5 * cond_home);
  RC_CHECK(cond_min < 1e-3);
  RC_CHECK(cond_home > 0.0);
  RC_CHECK(cond_min <= cond_home + 1e-12);
  RC_CHECK(cond_flat < cond_home);
}
