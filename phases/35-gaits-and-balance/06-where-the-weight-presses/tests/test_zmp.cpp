#include <rc/test/rc_test.hpp>

#include <cmath>

#include "rc/legged/balance_force.hpp"
#include "solution.hpp"
#include <iomanip>
#include <iostream>

static void rec(const char* key, double value, int digits = 3) {
  std::cout << "@" << key << " " << std::fixed << std::setprecision(digits) << value << std::endl;
  std::cout.unsetf(std::ios::floatfield);
}
[[maybe_unused]] static void recs(const char* key, const std::string& value) { std::cout << "@" << key << " " << value << std::endl; }


namespace {

void feet4(double feet[4][2]) {
  feet[0][0] = 0.19;  feet[0][1] = 0.15;
  feet[1][0] = 0.19;  feet[1][1] = -0.15;
  feet[2][0] = -0.19; feet[2][1] = 0.15;
  feet[3][0] = -0.19; feet[3][1] = -0.15;
}

}  // namespace

RC_TEST("the centre of pressure is the force weighted mean of the feet, and equals the zero moment point when the forces are chosen to give it") {
  double feet[4][2];
  feet4(feet);
  const double equal[4] = {10, 10, 10, 10};
  double c[2];
  RC_CHECK(cop_from_forces(feet, equal, c));
  RC_CHECK_NEAR(c[0], 0.0, 1e-12);
  RC_CHECK_NEAR(c[1], 0.0, 1e-12);
  const double zero[4] = {0, 0, 0, 0};
  RC_CHECK(!cop_from_forces(feet, zero, c));
  // a body of 12 kg accelerating forward at 3 m/s2, 0.27 m up: the point is behind the centre; give the feet the forces that put the centre of pressure there
  const double com[2] = {0.0, 0.0}, a[2] = {3.0, 0.0};
  double z[2];
  zmp_from_com(com, 0.27, a, z);
  const double W = 12.0 * 9.81;
  double f[4];
  const bool all[4] = {true, true, true, true};
  RC_CHECK(foot_forces(feet, all, W, W * z[1], -W * z[0], f));
  double cop[2];
  RC_CHECK(cop_from_forces(feet, f, cop));
  rec("a", a[0], 0);
  rec("h", 0.27, 2);
  rec("shift_mm", -z[0] * 1000.0, 0);
  rec("cop_err", std::hypot(cop[0] - z[0], cop[1] - z[1]), 9);
  RC_CHECK_NEAR(z[0], -0.27 / 9.81 * 3.0, 1e-12);
  RC_CHECK_NEAR(cop[0], z[0], 1e-9);
  RC_CHECK_NEAR(cop[1], z[1], 1e-9);
}

RC_TEST("the greatest acceleration is g times the distance to the edge over the height, and agrees with the tipping limit") {
  double feet[4][2];
  feet4(feet);
  const bool all[4] = {true, true, true, true};
  const double com[2] = {0.0, 0.0}, h = 0.27;
  const double fwd[2] = {1.0, 0.0}, back[2] = {-1.0, 0.0}, side[2] = {0.0, 1.0};
  const double a_fwd = max_accel(feet, all, com, h, fwd), a_back = max_accel(feet, all, com, h, back), a_side = max_accel(feet, all, com, h, side);
  rec("a_fwd", a_fwd, 2);
  rec("a_back", a_back, 2);
  rec("a_side", a_side, 2);
  RC_CHECK_NEAR(a_fwd, 9.81 * 0.19 / h, 1e-6);
  RC_CHECK_NEAR(a_side, 9.81 * 0.15 / h, 1e-6);
  RC_CHECK_NEAR(a_fwd, a_back, 1e-6);
  // the formula of the mobile manipulator lesson: g times the distance from the centre to the edge over its height
  const double tip = 9.81 * 0.5 * (2.0 * 0.15) / h;
  rec("agree", std::fabs(a_side - tip) < 1e-6 ? 1 : 0, 0);
  RC_CHECK_NEAR(a_side, tip, 1e-6);
}

RC_TEST("with three feet and a centred body there is no margin, and with two feet there never is") {
  double feet[4][2];
  feet4(feet);
  const double com[2] = {0.0, 0.0};
  const bool three[4] = {true, true, true, false};   // a rear right foot lifted
  const bool diag[4] = {true, false, false, true};
  const double m3 = support_margin(feet, three, com), m2 = support_margin(feet, diag, com);
  rec("m3", std::fabs(m3), 3);
  rec("m2", std::fabs(m2), 3);
  const double u[2] = {1.0, 0.0};
  rec("a_three", max_accel(feet, three, com, 0.27, u), 2);
  RC_CHECK_NEAR(m3, 0.0, 1e-12);
  RC_CHECK_NEAR(max_accel(feet, three, com, 0.27, u), 0.0, 1e-6);
  RC_CHECK(m2 <= 0.0);
  RC_CHECK_NEAR(max_accel(feet, diag, com, 0.27, u), 0.0, 1e-12);
  // shifted to the middle of the three feet there is room
  const double shifted[2] = {0.05, 0.03};
  RC_CHECK(support_margin(feet, three, shifted) > 0.0);
  RC_CHECK(max_accel(feet, three, shifted, 0.27, u) > 0.5);
}
