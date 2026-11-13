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

double dist(const Pose& a, const Pose& b) {
  return std::sqrt((a.p[0] - b.p[0]) * (a.p[0] - b.p[0]) + (a.p[1] - b.p[1]) * (a.p[1] - b.p[1]) + (a.p[2] - b.p[2]) * (a.p[2] - b.p[2]));
}

double rot_gap(const Pose& a, const Pose& b) {
  double w = 0.0;
  for (int i = 0; i < 3; ++i)
    for (int j = 0; j < 3; ++j) w = std::fmax(w, std::fabs(a.r[i][j] - b.r[i][j]));
  return w;
}

}  // namespace

RC_TEST("with every joint at zero the UR5's tool is where the sums of its lengths put it") {
  const ArmTable t = ur5();
  const Pose tool = forward(t, std::vector<double>(6, 0.0), Pose(), Pose());
  rec("ur5_x", tool.p[0], 5);
  rec("ur5_y", tool.p[1], 5);
  rec("ur5_z", tool.p[2], 5);
  // a2 + a3, minus d4 and d6, and d1 less d5: read off the table, not computed by the code under test.
  RC_CHECK_NEAR(tool.p[0], -0.425 - 0.39225, 1e-12);
  RC_CHECK_NEAR(tool.p[1], -(0.10915 + 0.0823), 1e-12);
  RC_CHECK_NEAR(tool.p[2], 0.089159 - 0.09465, 1e-12);
}

RC_TEST("the Panda's flange at zero is at its published place, and its zero pose is not one it can take") {
  const ArmTable t = panda();
  const std::vector<double> zero(7, 0.0);
  const Pose flange = forward(t, zero, Pose(), trans(0.0, 0.0, 0.107));
  rec("pan_x", flange.p[0], 3);
  rec("pan_y", flange.p[1], 3);
  rec("pan_z", flange.p[2], 3);
  rec("pan_in", within_limits(t, zero) ? 1 : 0, 0);
  RC_CHECK_NEAR(flange.p[0], 0.0825 - 0.0825 + 0.088, 1e-12);
  RC_CHECK_NEAR(flange.p[1], 0.0, 1e-12);
  RC_CHECK_NEAR(flange.p[2], 0.333 + 0.316 + 0.384 - 0.107, 1e-12);
  RC_CHECK(!within_limits(t, zero));
  std::vector<double> ok(7, 0.0);
  ok[3] = -1.5;
  ok[5] = 1.5;
  RC_CHECK(within_limits(t, ok));
}

RC_TEST("turning the first joint turns the whole arm about the base axis") {
  const ArmTable t = ur5();
  std::vector<double> q(6, 0.0);
  q[1] = -0.7;
  q[2] = 1.1;
  q[4] = 0.4;
  const Pose a = forward(t, q, Pose(), Pose());
  q[0] = 0.9;
  const Pose b = forward(t, q, Pose(), Pose());
  const Pose turned = compose(rot_z(0.9), a);
  rec("turn_err", dist(b, turned), 12);
  RC_CHECK(dist(b, turned) < 1e-12);
  RC_CHECK(rot_gap(b, turned) < 1e-12);
}

RC_TEST("a joint offset is the same as turning the joint by it") {
  ArmTable t = ur5();
  std::vector<double> q(6, 0.3);
  const Pose a = forward(t, q, Pose(), Pose());
  t.rows[2].theta_offset = 0.25;
  q[2] -= 0.25;
  const Pose b = forward(t, q, Pose(), Pose());
  RC_CHECK(dist(a, b) < 1e-12);
  RC_CHECK(rot_gap(a, b) < 1e-12);
}

RC_TEST("the tool offset moves the tool along the flange's own axis") {
  const ArmTable t = ur5();
  std::vector<double> q(6, 0.2);
  q[3] = -0.5;
  const Pose flange = forward(t, q, Pose(), Pose());
  const Pose tip = forward(t, q, Pose(), trans(0.0, 0.0, 0.15));
  rec("tip_move", dist(flange, tip), 3);
  RC_CHECK_NEAR(dist(flange, tip), 0.15, 1e-12);
  const double axis[3] = {flange.r[0][2], flange.r[1][2], flange.r[2][2]};
  for (int i = 0; i < 3; ++i) RC_CHECK_NEAR(tip.p[i] - flange.p[i], 0.15 * axis[i], 1e-12);
  const std::vector<Pose> frames = link_frames(t, q, Pose());
  RC_CHECK(frames.size() == 6);
  RC_CHECK(dist(frames.back(), flange) < 1e-12);
}

RC_TEST("roll pitch yaw gives the rotation back, and near a pitch of ninety degrees a hair of rotation is a jump in roll") {
  for (int i = 0; i < 50; ++i) {
    Rpy e;
    e.roll = -3.0 + 0.12 * i;
    e.pitch = -1.4 + 0.056 * i;
    e.yaw = 2.9 - 0.11 * i;
    const Pose r = rotation_from_rpy(e);
    RC_CHECK(rot_gap(rotation_from_rpy(rpy_from_rotation(r)), r) < 1e-12);
  }
  // At exactly the quarter turn the round trip still returns the rotation.
  Rpy lock;
  lock.roll = 0.4;
  lock.pitch = kPi / 2.0;
  lock.yaw = 1.0;
  const Pose rl = rotation_from_rpy(lock);
  RC_CHECK(rot_gap(rotation_from_rpy(rpy_from_rotation(rl)), rl) < 1e-9);
  // A hair away from it (1e-4 rad of pitch) roll and yaw are separate again, and swing by a whole radian for a rotation that has hardly moved.
  Rpy near = lock;
  near.pitch = kPi / 2.0 - 1e-4;
  const Rpy rn = rpy_from_rotation(rotation_from_rpy(near));
  const Rpy rl2 = rpy_from_rotation(rl);
  const double jump = std::fabs(rn.roll - rl2.roll) * 180.0 / kPi;
  rec("rpy_jump", jump, 0);
  RC_CHECK(rot_gap(rotation_from_rpy(near), rl) < 1e-3);
  RC_CHECK(jump > 30.0);
}
