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
    {"mass", 12.0},
    {"w_leg", 29.43},
    {"h_low", 0.2},
    {"h_mid", 0.3},
    {"h_high", 0.4},
    {"tk_low", 5.4345983375038855},
    {"th_low", 0.0},
    {"tk_mid", 4.325308987806535},
    {"th_mid", 0.0},
    {"tk_high", 1.8844394630764854},
    {"th_high", 0.0},
    {"err_f", 3.552713678800501e-15},
    {"v_foot", 0.5},
    {"rate_mid", 3.4020690871988593},
    {"rate_high", 15.47646465068274},
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

void pose_for_height(const LegGeom& g, double h, double q[3]) { leg_ik(g, 0.0, g.l0, -h, -1, q); }

}  // namespace

RC_TEST("the Jacobian is the derivative of the forward kinematics") {
  LegGeom g;
  const double q[3] = {0.15, 0.7, -1.4};
  double j[3][3];
  leg_jacobian(g, q[0], q[1], q[2], j);
  const double h = 1e-6;
  const FootPos p0 = leg_fk(g, q[0], q[1], q[2]);
  for (int c = 0; c < 3; ++c) {
    double qq[3] = {q[0], q[1], q[2]};
    qq[c] += h;
    const FootPos p1 = leg_fk(g, qq[0], qq[1], qq[2]);
    RC_CHECK_NEAR((p1.x - p0.x) / h, j[0][c], 1e-5);
    RC_CHECK_NEAR((p1.y - p0.y) / h, j[1][c], 1e-5);
    RC_CHECK_NEAR((p1.z - p0.z) / h, j[2][c], 1e-5);
  }
  LegGeom r = g;
  r.side = -1;
  leg_jacobian(r, q[0], q[1], q[2], j);
  const FootPos pr = leg_fk(r, q[0], q[1], q[2]);
  double qq[3] = {q[0] + h, q[1], q[2]};
  const FootPos pr1 = leg_fk(r, qq[0], qq[1], qq[2]);
  RC_CHECK_NEAR((pr1.y - pr.y) / h, j[1][0], 1e-5);
}

RC_TEST("holding a body up takes less torque from the knee the straighter the leg, and the torques give the force back") {
  LegGeom g;
  const double mass = 12.0, w = mass * 9.81 / 4.0;
  const double ground[3] = {0.0, 0.0, w};
  const double hs[3] = {0.20, 0.30, 0.40};
  double tk[3], th[3], worst = 0.0;
  for (int i = 0; i < 3; ++i) {
    double q[3], j[3][3], tau[3], back[3];
    pose_for_height(g, hs[i], q);
    leg_jacobian(g, q[0], q[1], q[2], j);
    stand_torques(j, ground, tau);
    th[i] = std::fabs(tau[1]);
    tk[i] = std::fabs(tau[2]);
    RC_CHECK(foot_force_from_torques(j, tau, back));
    for (int k = 0; k < 3; ++k) worst = std::fmax(worst, std::fabs(back[k] - ground[k]));
    // the knee's torque is the weight times the shank's lean from the vertical
    RC_CHECK_NEAR(tk[i], w * g.l2 * std::fabs(std::sin(q[1] + q[2])), 1e-9);
  }
  rec("mass", mass, 0);
  rec("w_leg", w, 1);
  rec("h_low", hs[0], 2);
  rec("h_mid", hs[1], 2);
  rec("h_high", hs[2], 2);
  rec("tk_low", tk[0], 2);
  rec("th_low", th[0], 2);
  rec("tk_mid", tk[1], 2);
  rec("th_mid", th[1], 2);
  rec("tk_high", tk[2], 2);
  rec("th_high", th[2], 2);
  rec("err_f", worst, 9);
  RC_CHECK(tk[0] > tk[1] && tk[1] > tk[2]);
  RC_CHECK(worst < 1e-9);
}

RC_TEST("a foot velocity needs joint rates that grow as the leg straightens, and a straight leg has none") {
  LegGeom g;
  const double v[3] = {0.0, 0.0, -0.5};
  double rate_mid = 0.0, rate_high = 0.0;
  for (int i = 0; i < 2; ++i) {
    double q[3], j[3][3], r[3];
    pose_for_height(g, i == 0 ? 0.30 : 0.415, q);
    leg_jacobian(g, q[0], q[1], q[2], j);
    RC_CHECK(joint_rates(j, v, r));
    (i == 0 ? rate_mid : rate_high) = std::fabs(r[2]);
  }
  rec("v_foot", 0.5, 1);
  rec("rate_mid", rate_mid, 1);
  rec("rate_high", rate_high, 1);
  RC_CHECK(rate_high > 3.0 * rate_mid);
  double q[3], j[3][3], r[3];
  RC_CHECK(leg_ik(g, 0.0, g.l0, -(g.l1 + g.l2) + 1e-9, -1, q));
  leg_jacobian(g, q[0], q[1], q[2], j);
  RC_CHECK(!joint_rates(j, v, r));
}
