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

MobileManipulator make() {
  MobileManipulator m;
  m.arm = elbow_arm();
  m.mount = trans(0.15, 0.0, 0.30);
  m.tool = trans(0.0, 0.0, 0.10);
  return m;
}

}  // namespace

RC_TEST("with the base at the origin the tool is where the arm alone says, shifted by the mount") {
  const MobileManipulator m = make();
  const std::vector<double> q(6, 0.0);
  BasePose b;
  const Pose alone = forward(m.arm, q, m.mount, m.tool);
  const Pose t = tool_in_world(m, b, q);
  for (int k = 0; k < 3; ++k) RC_CHECK_NEAR(t.p[k], alone.p[k], 1e-12);
  const Pose nomount = forward(m.arm, q, Pose(), m.tool);
  RC_CHECK_NEAR(t.p[0], nomount.p[0] + 0.15, 1e-12);
  RC_CHECK_NEAR(t.p[2], nomount.p[2] + 0.30, 1e-12);
}

RC_TEST("moving the base moves the tool with it, and turning the base turns the tool about the base's centre") {
  const MobileManipulator m = make();
  const std::vector<double> q(6, 0.0);
  BasePose o, b;
  b.x = 2.0;
  b.y = 1.0;
  b.heading = 1.5707963267948966;
  const Pose t0 = tool_in_world(m, o, q), t1 = tool_in_world(m, b, q);
  rec("bx", b.x, 1);
  rec("by", b.y, 1);
  rec("bh_deg", 90, 0);
  rec("tx", t1.p[0], 3);
  rec("ty", t1.p[1], 3);
  rec("tz", t1.p[2], 3);
  // a quarter turn about the vertical takes (x, y) to (-y, x), then the base's place is added
  RC_CHECK_NEAR(t1.p[0], b.x - t0.p[1], 1e-9);
  RC_CHECK_NEAR(t1.p[1], b.y + t0.p[0], 1e-9);
  RC_CHECK_NEAR(t1.p[2], t0.p[2], 1e-9);
}

RC_TEST("a point in the world goes into the arm's frame and back") {
  const MobileManipulator m = make();
  BasePose b;
  b.x = -1.0;
  b.y = 0.5;
  b.heading = 0.7;
  const double w[3] = {-0.4, 0.9, 0.2};
  double a[3], back[3];
  world_to_arm(m, b, w, a);
  apply(arm_base_in_world(m, b), a, back);
  double e = 0.0;
  for (int k = 0; k < 3; ++k) e = std::fmax(e, std::fabs(back[k] - w[k]));
  rec("round_err", e, 9);
  RC_CHECK(e < 1e-12);
  // the target straight ahead of the arm's mount is on the arm's x axis
  const Pose mount_w = arm_base_in_world(m, b);
  const double ahead[3] = {mount_w.p[0] + 0.5 * std::cos(b.heading), mount_w.p[1] + 0.5 * std::sin(b.heading), mount_w.p[2]};
  world_to_arm(m, b, ahead, a);
  RC_CHECK_NEAR(a[0], 0.5, 1e-12);
  RC_CHECK_NEAR(a[1], 0.0, 1e-12);
}

RC_TEST("a heading error is multiplied by the reach, and a position error is not") {
  BasePose b;
  const double hand[3] = {0.9, 0.0, 0.3};
  const double dpos = 0.005, dh = 1.0 * 3.14159265358979323846 / 180.0;
  rec("dpos_mm", dpos * 1000.0, 0);
  rec("dh_deg", 1, 0);
  rec("lever", 0.9, 1);
  rec("dh_mm", 1000.0 * hand_error_from_base(b, hand, 0.0, dh), 1);
  RC_CHECK_NEAR(hand_error_from_base(b, hand, dpos, 0.0), dpos, 1e-12);
  RC_CHECK_NEAR(hand_error_from_base(b, hand, 0.0, dh), 0.9 * dh, 1e-12);
  RC_CHECK(hand_error_from_base(b, hand, dpos, dh) > hand_error_from_base(b, hand, dpos, 0.0));
}
