#include <rc/test/rc_test.hpp>
#include <sstream>
#include <cstring>

#include <cmath>
#include <iostream>

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
    {"conv_gap", 0.17031558728146975},
    {"inv_err", 2.220446049250313e-16},
    {"wrong_inv", 0.34519729968597423},
    {"drift", 1.2605161359147132e-10},
    {"drift_fixed", 2.220446049250313e-16},
    {"ur5_z1", 0.089159},
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

double pose_gap(const Pose& a, const Pose& b) {
  double w = 0.0;
  for (int i = 0; i < 3; ++i) {
    w = std::fmax(w, std::fabs(a.p[i] - b.p[i]));
    for (int j = 0; j < 3; ++j) w = std::fmax(w, std::fabs(a.r[i][j] - b.r[i][j]));
  }
  return w;
}

Pose awkward() {
  return compose(compose(trans(0.3, -0.2, 0.4), rot_z(0.7)), compose(rot_y(-0.4), rot_x(1.1)));
}

}  // namespace

RC_TEST("a row of the table is its four elementary moves, in the order of its convention") {
  const double a = 0.3, alpha = 0.6, d = 0.2, theta = -0.9;
  const Pose s = compose(compose(rot_z(theta), trans(0, 0, d)), compose(trans(a, 0, 0), rot_x(alpha)));
  const Pose m = compose(compose(rot_x(alpha), trans(a, 0, 0)), compose(rot_z(theta), trans(0, 0, d)));
  RC_CHECK(pose_gap(dh_standard(a, alpha, d, theta), s) < 1e-12);
  RC_CHECK(pose_gap(dh_modified(a, alpha, d, theta), m) < 1e-12);
  const double gap = std::hypot(std::hypot(s.p[0] - m.p[0], s.p[1] - m.p[1]), s.p[2] - m.p[2]);
  rec("conv_gap", gap, 3);
  RC_CHECK(gap > 0.1);
}

RC_TEST("the inverse of a pose undoes it, and one that only transposes the rotation does not") {
  const Pose t = awkward();
  const Pose id = compose(t, inverse(t));
  RC_CHECK(pose_gap(id, Pose()) < 1e-12);
  rec("inv_err", pose_gap(id, Pose()), 16);
  Pose wrong = inverse(t);
  for (int i = 0; i < 3; ++i) wrong.p[i] = -t.p[i];
  const Pose bad = compose(t, wrong);
  const double off = std::hypot(std::hypot(bad.p[0], bad.p[1]), bad.p[2]);
  rec("wrong_inv", off, 2);
  RC_CHECK(off > 0.1);
}

RC_TEST("a point moves by the rotation and then the translation") {
  const Pose t = compose(trans(1.0, 2.0, 3.0), rot_z(kPi / 2.0));
  const double in[3] = {1.0, 0.0, 0.0};
  double out[3];
  apply(t, in, out);
  RC_CHECK_NEAR(out[0], 1.0, 1e-12);
  RC_CHECK_NEAR(out[1], 3.0, 1e-12);
  RC_CHECK_NEAR(out[2], 3.0, 1e-12);
}

RC_TEST("a rotation composed a million times drifts, and is put back by orthonormalising") {
  Pose acc;
  const Pose step = compose(rot_x(0.0123), compose(rot_y(-0.0071), rot_z(0.0049)));
  for (int i = 0; i < 1000000; ++i) acc = compose(acc, step);
  rec("drift", rotation_error(acc), 12);
  const Pose fixed = orthonormalised(acc);
  rec("drift_fixed", rotation_error(fixed), 16);
  RC_CHECK(rotation_error(acc) > 1e-13);
  RC_CHECK(rotation_error(fixed) < 1e-14);
  RC_CHECK(rotation_error(acc) < 1e-6);
}

RC_TEST("the first frame of the UR5 is at the height of its shoulder with its axis turned a quarter turn") {
  const ArmTable t = ur5();
  RC_CHECK(t.rows.size() == 6);
  RC_CHECK(t.convention == kStandard);
  const Pose f1 = dh_standard(t.rows[0].a, t.rows[0].alpha, t.rows[0].d, 0.0);
  rec("ur5_z1", f1.p[2], 6);
  RC_CHECK_NEAR(f1.p[0], 0.0, 1e-12);
  RC_CHECK_NEAR(f1.p[2], 0.089159, 1e-12);
  // The z axis of frame 1 is the second joint's axis: horizontal, along -y, after the quarter turn of alpha.
  RC_CHECK_NEAR(f1.r[1][2], -1.0, 1e-12);
  RC_CHECK_NEAR(f1.r[2][2], 0.0, 1e-12);
}

RC_TEST("the Panda has seven joints, in the modified convention, and every joint has limits that are an interval") {
  const ArmTable t = panda();
  RC_CHECK(t.rows.size() == 7);
  RC_CHECK(t.convention == kModified);
  for (std::size_t i = 0; i < t.rows.size(); ++i) RC_CHECK(t.rows[i].lo < t.rows[i].hi);
  RC_CHECK(!t.source.empty());
}
