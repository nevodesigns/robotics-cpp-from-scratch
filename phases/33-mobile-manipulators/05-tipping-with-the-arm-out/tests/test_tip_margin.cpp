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
  m.mount = trans(0.0, 0.0, 0.10);
  m.tool = trans(0.0, 0.0, 0.10);
  return m;
}

std::vector<double> pose_q(const MobileManipulator& m, double x, double y, double z) {
  Pose p;
  p.r[1][1] = -1.0;
  p.r[2][2] = -1.0;
  p.p[0] = x;
  p.p[1] = y;
  p.p[2] = z;
  const std::vector<std::vector<double> > sols = ik_spherical(m.arm, p, Pose(), m.tool);
  if (sols.empty()) return std::vector<double>();
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

MassModel masses(double payload) {
  MassModel mm;
  const double l[6] = {3.0, 2.5, 2.0, 0.8, 0.6, 0.4};
  mm.link_mass.assign(l, l + 6);
  mm.payload = payload;
  return mm;
}

const double kHalfWheelbase = 0.30, kHalfTrack = 0.22;

}  // namespace

RC_TEST("with no arm mass and no load the centre of mass is the base's, and masses pull it by their moments") {
  const MobileManipulator m = make();
  const std::vector<double> q = pose_q(m, 0.55, 0.0, 0.30);
  RC_CHECK(!q.empty());
  BasePose b;
  b.x = 1.0;
  b.y = 2.0;
  b.heading = 0.5;
  MassModel none;
  none.link_mass.assign(6, 0.0);
  double c[3];
  com_world(m, b, q, none, c);
  const Pose bp = base_pose(b);
  double expect[3];
  apply(bp, none.base_com, expect);
  for (int k = 0; k < 3; ++k) RC_CHECK_NEAR(c[k], expect[k], 1e-12);
  // a payload at the tool of the same mass as the base halves the way to the tool
  MassModel equal = none;
  equal.payload = none.base_mass;
  double c2[3];
  com_world(m, b, q, equal, c2);
  const Pose tool = tool_in_world(m, b, q);
  for (int k = 0; k < 3; ++k) RC_CHECK_NEAR(c2[k], 0.5 * (expect[k] + tool.p[k]), 1e-9);
}

RC_TEST("reaching and carrying take the centre of mass towards the edge and cut the braking limit") {
  const MobileManipulator m = make();
  BasePose b;
  const std::vector<double> qt = pose_q(m, 0.20, 0.0, 0.35), qr = pose_q(m, 0.65, 0.0, 0.30);
  RC_CHECK(!qt.empty() && !qr.empty());
  double ct[3], cr[3], cl[3];
  com_world(m, b, qt, masses(0.0), ct);
  com_world(m, b, qr, masses(0.0), cr);
  com_world(m, b, qr, masses(5.0), cl);
  const TipLimits tt = tip_limits(b, ct, kHalfWheelbase, kHalfTrack);
  const TipLimits tr = tip_limits(b, cr, kHalfWheelbase, kHalfTrack);
  const TipLimits tl = tip_limits(b, cl, kHalfWheelbase, kHalfTrack);
  rec("com_x_tuck", tt.com_x, 3);
  rec("h_tuck", tt.com_z, 3);
  rec("brake_tuck", tt.brake, 2);
  rec("reach_m", 0.65, 2);
  rec("com_x_reach", tr.com_x, 3);
  rec("h_reach", tr.com_z, 3);
  rec("brake_reach", tr.brake, 2);
  rec("load", 5, 0);
  rec("brake_load", tl.brake, 2);
  RC_CHECK(tr.com_x > tt.com_x);
  RC_CHECK(tr.brake < tt.brake);
  RC_CHECK(tl.brake < tr.brake);
  RC_CHECK(tl.accel > tr.accel);
  const double slip = 0.6 * 9.81;  // rubber on a dry floor (typical)
  rec("slip_limit", slip, 2);
  RC_CHECK(tt.brake > slip && tl.brake < slip);
  const double v0 = 1.0;
  rec("v0", v0, 1);
  rec("stop_m", stopping_distance(v0, tl.brake), 3);
  RC_CHECK_NEAR(stopping_distance(v0, tl.brake), v0 * v0 / (2.0 * tl.brake), 1e-12);
}

RC_TEST("reaching to one side cuts the limit on turning towards it, and a centred mass gives the 19-09 value") {
  const MobileManipulator m = make();
  BasePose b;
  const std::vector<double> qt = pose_q(m, 0.20, 0.0, 0.35), qs = pose_q(m, 0.0, 0.65, 0.30);
  RC_CHECK(!qt.empty() && !qs.empty());
  double ct[3], cs[3];
  com_world(m, b, qt, masses(0.0), ct);
  com_world(m, b, qs, masses(2.0), cs);
  const TipLimits tt = tip_limits(b, ct, kHalfWheelbase, kHalfTrack);
  const TipLimits ts = tip_limits(b, cs, kHalfWheelbase, kHalfTrack);
  rec("side_reach", ts.right_turn < ts.left_turn ? ts.right_turn : ts.left_turn, 2);
  rec("side_tuck", tt.right_turn < tt.left_turn ? tt.right_turn : tt.left_turn, 2);
  RC_CHECK(ts.left_turn > ts.right_turn || ts.left_turn < ts.right_turn);
  const double lo_s = ts.right_turn < ts.left_turn ? ts.right_turn : ts.left_turn;
  const double lo_t = tt.right_turn < tt.left_turn ? tt.right_turn : tt.left_turn;
  RC_CHECK(lo_s < lo_t);
  // a mass over the middle of the wheels gives g times half the track over the height, the formula of 19-09
  const double centred[3] = {0.0, 0.0, 0.40};
  const TipLimits c = tip_limits(b, centred, kHalfWheelbase, kHalfTrack);
  RC_CHECK_NEAR(c.left_turn, rc::safety::tip_accel(2.0 * kHalfTrack, 0.40), 1e-9);
  RC_CHECK_NEAR(c.right_turn, c.left_turn, 1e-12);
  RC_CHECK_NEAR(c.brake, c.accel, 1e-12);
}
