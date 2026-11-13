#include <rc/test/rc_test.hpp>

#include <cmath>

#include "rc/arm/ik_closed.hpp"
#include "solution.hpp"
#include <iomanip>
#include <iostream>

static void rec(const char* key, double value, int digits = 3) {
  std::cout << "@" << key << " " << std::fixed << std::setprecision(digits) << value << std::endl;
  std::cout.unsetf(std::ios::floatfield);
}
[[maybe_unused]] static void recs(const char* key, const std::string& value) { std::cout << "@" << key << " " << value << std::endl; }


namespace {

MobileManipulator make() {
  MobileManipulator m;
  m.arm = elbow_arm();
  for (std::size_t i = 0; i < m.arm.rows.size(); ++i) {
    m.arm.rows[i].lo = -3.0;
    m.arm.rows[i].hi = 3.0;
  }
  m.mount = trans(0.15, 0.0, 0.30);
  m.tool = trans(0.0, 0.0, 0.10);
  return m;
}

// A start pose: the tool pointing down in front of the arm.
std::vector<double> start_q(const MobileManipulator& m) {
  Pose p;
  p.r[1][1] = -1.0;
  p.r[2][2] = -1.0;
  p.p[0] = 0.45;
  p.p[1] = 0.0;
  p.p[2] = 0.10;
  const std::vector<std::vector<double> > sols = ik_spherical(m.arm, p, Pose(), m.tool);
  if (sols.empty()) return std::vector<double>(6, 0.0);
  std::vector<double> best = sols[0];
  double bn = 1e9;
  for (std::size_t i = 0; i < sols.size(); ++i) {
    double n = 0.0;
    for (std::size_t k = 0; k < 6; ++k) n += sols[i][k] * sols[i][k];
    if (n < bn) {
      bn = n;
      best = sols[i];
    }
  }
  return best;
}

}  // namespace

RC_TEST("the whole-body Jacobian agrees with a small move of the base and of the joints") {
  const MobileManipulator m = make();
  const std::vector<double> q = start_q(m);
  BasePose b;
  b.x = 0.3;
  b.y = -0.2;
  b.heading = 0.4;
  const WbJacobian j = wb_jacobian(m, b, q);
  const double h = 1e-6;
  const Pose p0 = tool_in_world(m, b, q);
  // driving
  BasePose b2 = b;
  b2.x += h * std::cos(b.heading);
  b2.y += h * std::sin(b.heading);
  const Pose p1 = tool_in_world(m, b2, q);
  for (int k = 0; k < 3; ++k) RC_CHECK_NEAR((p1.p[k] - p0.p[k]) / h, j.m[k][0], 1e-4);
  // turning
  b2 = b;
  b2.heading += h;
  const Pose p2 = tool_in_world(m, b2, q);
  for (int k = 0; k < 3; ++k) RC_CHECK_NEAR((p2.p[k] - p0.p[k]) / h, j.m[k][1], 1e-4);
  // the second joint
  std::vector<double> q2 = q;
  q2[1] += h;
  const Pose p3 = tool_in_world(m, b, q2);
  for (int k = 0; k < 3; ++k) RC_CHECK_NEAR((p3.p[k] - p0.p[k]) / h, j.m[k][3], 1e-4);
}

RC_TEST("the rates give the hand its velocity, and a dearer base moves less") {
  const MobileManipulator m = make();
  const std::vector<double> q = start_q(m);
  BasePose b;
  const WbJacobian j = wb_jacobian(m, b, q);
  const double xi[6] = {0.1, 0.05, 0.0, 0.0, 0.0, 0.0};
  const std::vector<double> r1 = wb_rates(j, xi, 1.0, 1e-3);
  for (int row = 0; row < 3; ++row) {
    double v = 0.0;
    for (int c = 0; c < 8; ++c) v += j.m[row][c] * r1[c];
    RC_CHECK_NEAR(v, xi[row], 2e-3);
  }
  const std::vector<double> r_dear = wb_rates(j, xi, 100.0, 1e-3);
  const std::vector<double> r_cheap = wb_rates(j, xi, 0.01, 1e-3);
  RC_CHECK(std::fabs(r_dear[0]) < std::fabs(r1[0]));
  RC_CHECK(std::fabs(r_cheap[0]) > std::fabs(r1[0]));
}

RC_TEST("a line farther than the arm can reach is drawn by the arm and base together and not by the arm alone") {
  const MobileManipulator m = make();
  const std::vector<double> q = start_q(m);
  BasePose b;
  const double goal[3] = {2.10, 0.0, 0.40};
  const WbRun arm = run_line(m, b, q, goal, 0.15, 1.0, false, 0.02);
  const WbRun wb = run_line(m, b, q, goal, 0.15, 1.0, true, 0.02);
  const WbRun hi = run_line(m, b, q, goal, 0.15, 30.0, true, 0.02);
  const WbRun lo = run_line(m, b, q, goal, 0.15, 0.03, true, 0.02);
  rec("len", 1.5, 1);
  rec("speed", 0.15, 2);
  rec("gap_arm", arm.end_gap, 2);
  rec("manip_arm", arm.min_manip, 4);
  rec("gap_wb", wb.end_gap, 3);
  rec("travel_wb", wb.base_travel, 2);
  rec("manip_wb", wb.min_manip, 4);
  rec("w_hi", 30, 0);
  rec("travel_hi", hi.base_travel, 2);
  rec("w_lo", 33, 0);
  rec("travel_lo", lo.base_travel, 2);
  RC_CHECK(!arm.reached && arm.end_gap > 0.3);
  RC_CHECK(wb.reached);
  RC_CHECK(wb.max_err < 0.02);
  RC_CHECK(wb.min_manip > arm.min_manip);
  RC_CHECK(wb.base_travel > 0.3 && wb.base_travel < 2.5);
  RC_CHECK(hi.base_travel < wb.base_travel);
}
