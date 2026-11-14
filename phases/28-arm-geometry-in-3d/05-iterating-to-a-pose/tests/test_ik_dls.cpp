#include <rc/test/rc_test.hpp>
#include <sstream>
#include <cstring>

#include <cmath>
#include <iostream>

#include "rc/arm/ik_closed.hpp"
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
    {"n", 40.0},
    {"near_ur5", 37.0},
    {"far_ur5", 31.0},
    {"iters_ur5", 21.405405405405407},
    {"near_panda", 36.0},
    {"plain_step", 20.613147160258137},
    {"damped_step", 0.020549853664997064},
    {"unreach_err", 0.6352003633031207},
    {"in_eight", 29.0},
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

unsigned long long g_s = 99;
double unit() {
  g_s = g_s * 6364136223846793005ULL + 1442695040888963407ULL;
  return static_cast<double>((g_s >> 33) & 0xFFFFFFULL) / 16777216.0;
}

std::vector<double> random_q(const ArmTable& t) {
  std::vector<double> q(t.rows.size());
  for (std::size_t i = 0; i < q.size(); ++i) {
    const double lo = std::fmax(t.rows[i].lo, -2.5), hi = std::fmin(t.rows[i].hi, 2.5);
    q[i] = lo + (hi - lo) * unit();
  }
  return q;
}

std::vector<double> near_q(const std::vector<double>& q, double radius) {
  std::vector<double> out = q;
  for (std::size_t i = 0; i < q.size(); ++i) out[i] += radius * (2.0 * unit() - 1.0);
  return out;
}

bool same_angles(const std::vector<double>& a, const std::vector<double>& b) {
  for (std::size_t i = 0; i < a.size(); ++i)
    if (std::fabs(wrap_angle(a[i] - b[i])) > 1e-4) return false;
  return true;
}

}  // namespace

RC_TEST("the rotation vector is the axis times the angle, for small turns, large ones and a half turn") {
  double w[3];
  rotation_vector(rot_z(0.4), w);
  RC_CHECK_NEAR(w[2], 0.4, 1e-12);
  rotation_vector(compose(rot_x(1e-10), Pose()), w);
  RC_CHECK_NEAR(w[0], 1e-10, 1e-15);
  rotation_vector(rot_y(3.0), w);
  RC_CHECK_NEAR(w[1], 3.0, 1e-9);
  rotation_vector(rot_x(kPi), w);
  RC_CHECK_NEAR(std::fabs(w[0]), kPi, 1e-6);
  Pose id;
  double e[6];
  pose_error(id, id, e);
  for (int k = 0; k < 6; ++k) RC_CHECK_NEAR(e[k], 0.0, 1e-15);
}

RC_TEST("from a start near the answer the iteration reaches the pose, and from a start far away it often does") {
  const ArmTable u = ur5();
  const DlsConfig cfg;
  int near_ok = 0, far_ok = 0, n = 40;
  double iters = 0.0;
  for (int k = 0; k < n; ++k) {
    const std::vector<double> q = random_q(u);
    const Pose target = forward(u, q, Pose(), trans(0, 0, 0.1));
    const IkResult a = ik_dls(u, target, Pose(), trans(0, 0, 0.1), near_q(q, 0.6), cfg);
    const IkResult b = ik_dls(u, target, Pose(), trans(0, 0, 0.1), std::vector<double>(6, 0.0), cfg);
    if (a.ok) {
      ++near_ok;
      iters += a.iterations;
    }
    if (b.ok) ++far_ok;
  }
  rec("n", n, 0);
  rec("near_ur5", near_ok, 0);
  rec("far_ur5", far_ok, 0);
  rec("iters_ur5", near_ok ? iters / near_ok : 0.0, 1);
  RC_CHECK(near_ok >= 36);
  RC_CHECK(far_ok >= 10);
}

RC_TEST("a seven joint arm reaches the pose and does not need to be at the joint angles that made it") {
  const ArmTable p = panda();
  DlsConfig cfg;
  int ok = 0, same = 0, n = 40;
  for (int k = 0; k < n; ++k) {
    const std::vector<double> q = random_q(p);
    const Pose target = forward(p, q, Pose(), trans(0, 0, 0.107));
    const IkResult r = ik_dls(p, target, Pose(), trans(0, 0, 0.107), near_q(q, 0.6), cfg);
    if (r.ok) {
      ++ok;
      if (same_angles(r.q, q)) ++same;
    }
  }
  rec("near_panda", ok, 0);
  RC_CHECK(ok >= 34);
  RC_CHECK(same < ok);
}

RC_TEST("near a straight elbow an undamped step is enormous and a damped one is not") {
  const ArmTable u = ur5();
  std::vector<double> q(6, 0.0);
  q[1] = -1.0;
  q[2] = 0.002;  // the elbow all but straight: the arm can hardly lengthen
  q[3] = -0.5;
  q[4] = 1.0;
  const Jacobian j = jacobian(u, q, Pose(), Pose(), u.convention);
  // Whatever direction the arm cannot move in is somewhere in the six: ask for 1 cm (or 0.01 rad) along each, and take the worst step.
  double np = 0.0, nd = 0.0;
  for (int axis = 0; axis < 6; ++axis) {
    double e[6] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
    e[axis] = 0.01;
    const std::vector<double> plain = dls_step(j, e, 1e-8), damped = dls_step(j, e, 0.05);
    for (std::size_t i = 0; i < 6; ++i) {
      np = std::fmax(np, std::fabs(plain[i]));
      nd = std::fmax(nd, std::fabs(damped[i]));
    }
  }
  rec("plain_step", np, 2);
  rec("damped_step", nd, 3);
  RC_CHECK(np > 20.0 * nd);
  RC_CHECK(nd < 0.2);
}

RC_TEST("a pose out of reach is stopped by the limit of steps, with the tool as near as it got") {
  const ArmTable u = ur5();
  Pose far = forward(u, std::vector<double>(6, 0.3), Pose(), Pose());
  far.p[0] += 2.0;
  const IkResult r = ik_dls(u, far, Pose(), Pose(), std::vector<double>(6, 0.3), DlsConfig());
  rec("unreach_err", r.pos_err, 2);
  RC_CHECK(!r.ok);
  RC_CHECK(r.iterations == 200);
  RC_CHECK(r.pos_err > 0.3);
  RC_CHECK(r.pos_err < 2.0);
}

RC_TEST("on the arm with a spherical wrist the iteration finds one of the eight closed form solutions") {
  const ArmTable t = elbow_arm();
  DlsConfig cfg;
  int in_eight = 0, n = 30;
  for (int k = 0; k < n; ++k) {
    std::vector<double> q(6);
    for (int i = 0; i < 6; ++i) q[i] = -1.2 + 2.4 * unit();
    q[4] = 0.3 + 0.8 * unit();
    const Pose target = forward(t, q, Pose(), Pose());
    const IkResult r = ik_dls(t, target, Pose(), Pose(), near_q(q, 0.5), cfg);
    if (!r.ok) continue;
    const std::vector<std::vector<double> > all = ik_spherical(t, target, Pose(), Pose());
    for (std::size_t i = 0; i < all.size(); ++i)
      if (same_angles(all[i], r.q)) {
        ++in_eight;
        break;
      }
  }
  rec("in_eight", in_eight, 0);
  RC_CHECK(in_eight >= 24);
}
