#include <rc/test/rc_test.hpp>

#include <cmath>
#include <iostream>

#include "solution.hpp"
#include <iomanip>
#include <iostream>

static void rec(const char* key, double value, int digits = 3) {
  std::cout << "@" << key << " " << std::fixed << std::setprecision(digits) << value << std::endl;
  std::cout.unsetf(std::ios::floatfield);
}
[[maybe_unused]] static void recs(const char* key, const std::string& value) { std::cout << "@" << key << " " << value << std::endl; }


namespace {

std::vector<double> pose_q(std::size_t n, int k) {
  std::vector<double> q(n);
  for (std::size_t i = 0; i < n; ++i) q[i] = 0.9 * std::sin(1.7 * static_cast<double>(i + 1) + 0.37 * k);
  return q;
}

}  // namespace

RC_TEST("the analytic Jacobian is the finite difference one, for both arms, in both conventions' own rule") {
  const ArmTable u = ur5(), p = panda();
  double eu = 0.0, ep = 0.0;
  const int poses = 40;
  for (int k = 0; k < poses; ++k) {
    const std::vector<double> qu = pose_q(6, k), qp = pose_q(7, k);
    eu = std::fmax(eu, jacobian_gap(jacobian(u, qu, Pose(), trans(0, 0, 0.15), u.convention), numeric_jacobian(u, qu, Pose(), trans(0, 0, 0.15))));
    ep = std::fmax(ep, jacobian_gap(jacobian(p, qp, Pose(), trans(0, 0, 0.107), p.convention), numeric_jacobian(p, qp, Pose(), trans(0, 0, 0.107))));
  }
  rec("poses", poses, 0);
  rec("err_ur5", eu, 12);
  rec("err_panda", ep, 12);
  RC_CHECK(eu < 1e-6);
  RC_CHECK(ep < 1e-6);
}

RC_TEST("the rule of the other convention gives a Jacobian that is wrong") {
  const ArmTable p = panda();
  const std::vector<double> q = pose_q(7, 3);
  const Jacobian wrong = jacobian(p, q, Pose(), Pose(), kStandard);
  const double err = jacobian_gap(wrong, numeric_jacobian(p, q, Pose(), Pose()));
  rec("err_wrong", err, 2);
  RC_CHECK(err > 0.05);
}

RC_TEST("joint rates give the tool the velocity the Jacobian says") {
  const ArmTable u = ur5();
  const std::vector<double> q = pose_q(6, 5);
  const std::vector<double> qdot = pose_q(6, 9);
  const Jacobian j = jacobian(u, q, Pose(), Pose(), u.convention);
  double tw[6];
  tool_twist(j, qdot, tw);
  const double dt = 1e-6;
  std::vector<double> q2 = q;
  for (std::size_t i = 0; i < 6; ++i) q2[i] += qdot[i] * dt;
  const Pose a = forward(u, q, Pose(), Pose()), b = forward(u, q2, Pose(), Pose());
  for (int k = 0; k < 3; ++k) RC_CHECK_NEAR((b.p[k] - a.p[k]) / dt, tw[k], 1e-5);
}

RC_TEST("a tool offset changes the linear part of the Jacobian and not the angular part") {
  const ArmTable u = ur5();
  const std::vector<double> q = pose_q(6, 2);
  const Jacobian a = jacobian(u, q, Pose(), Pose(), u.convention);
  const Jacobian b = jacobian(u, q, Pose(), trans(0, 0, 0.15), u.convention);
  double lin = 0.0, ang = 0.0;
  for (int c = 0; c < 6; ++c) {
    for (int r = 0; r < 3; ++r) lin = std::fmax(lin, std::fabs(a.m[r][c] - b.m[r][c]));
    for (int r = 3; r < 6; ++r) ang = std::fmax(ang, std::fabs(a.m[r][c] - b.m[r][c]));
  }
  RC_CHECK(lin > 1e-3);
  RC_CHECK(ang < 1e-12);
}

RC_TEST("the joints and the tool do the same work: torque times joint rate is wrench times tool velocity") {
  const ArmTable p = panda();
  const std::vector<double> q = pose_q(7, 4), qdot = pose_q(7, 8);
  const Jacobian j = jacobian(p, q, Pose(), trans(0, 0, 0.107), p.convention);
  const double w[6] = {3.0, -2.0, 10.0, 0.5, -0.3, 0.2};
  const std::vector<double> tau = joint_torques(j, w);
  double tw[6];
  tool_twist(j, qdot, tw);
  double joint_power = 0.0, tool_power = 0.0;
  for (std::size_t i = 0; i < 7; ++i) joint_power += tau[i] * qdot[i];
  for (int r = 0; r < 6; ++r) tool_power += w[r] * tw[r];
  rec("power_err", std::fabs(joint_power - tool_power), 15);
  RC_CHECK_NEAR(joint_power, tool_power, 1e-12);
}

RC_TEST("a vertical force at the tool asks nothing of the first joint, whose axis is vertical") {
  const ArmTable u = ur5();
  const Jacobian j = jacobian(u, pose_q(6, 1), Pose(), Pose(), u.convention);
  const double w[6] = {0.0, 0.0, -10.0, 0.0, 0.0, 0.0};
  const std::vector<double> tau = joint_torques(j, w);
  rec("tau1", tau[0], 3);
  RC_CHECK_NEAR(tau[0], 0.0, 1e-12);
  RC_CHECK(std::fabs(tau[1]) > 0.1);
}
