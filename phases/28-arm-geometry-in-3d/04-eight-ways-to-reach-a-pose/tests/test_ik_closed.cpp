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
    {"poses", 40.0},
    {"avg_solutions", 8.0},
    {"found", 40.0},
    {"max_err", 4.596460756922437e-16},
    {"max_rot", 6.661338147750939e-16},
    {"far", 0.0},
    {"edge", 4.0},
    {"sing", 6.0},
    {"ur5_sph", 0.0},
    {"ur5_d5", 0.09465},
    {"limited", 1.0},
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

double pos_gap(const Pose& a, const Pose& b) {
  return std::sqrt((a.p[0] - b.p[0]) * (a.p[0] - b.p[0]) + (a.p[1] - b.p[1]) * (a.p[1] - b.p[1]) + (a.p[2] - b.p[2]) * (a.p[2] - b.p[2]));
}

double rot_gap(const Pose& a, const Pose& b) {
  double w = 0.0;
  for (int i = 0; i < 3; ++i)
    for (int j = 0; j < 3; ++j) w = std::fmax(w, std::fabs(a.r[i][j] - b.r[i][j]));
  return w;
}

bool same_angles(const std::vector<double>& a, const std::vector<double>& b) {
  for (std::size_t i = 0; i < a.size(); ++i)
    if (std::fabs(wrap_angle(a[i] - b[i])) > 1e-6) return false;
  return true;
}

std::vector<double> some_q(int k) {
  std::vector<double> q(6);
  q[0] = -2.8 + 0.31 * k;
  q[1] = 0.2 + 0.37 * std::sin(0.9 * k);
  q[2] = 0.4 + 0.5 * std::cos(1.3 * k);
  q[3] = 0.1 + 0.9 * std::sin(0.7 * k + 1.0);
  q[4] = 0.4 + 0.8 * std::fabs(std::sin(0.5 * k + 0.2)) ;
  q[5] = -1.5 + 0.23 * k;
  return q;
}

}  // namespace

RC_TEST("for many poses the angles that made the pose are among the solutions and every solution reaches it") {
  const ArmTable t = elbow_arm();
  const Pose tool = trans(0.0, 0.0, 0.05);
  int found = 0, poses = 0;
  double total = 0.0, worst_p = 0.0, worst_r = 0.0;
  for (int k = 0; k < 40; ++k) {
    const std::vector<double> q = some_q(k);
    const Pose target = forward(t, q, Pose(), tool);
    const std::vector<std::vector<double> > sols = ik_spherical(t, target, Pose(), tool);
    ++poses;
    total += static_cast<double>(sols.size());
    bool has = false;
    for (std::size_t i = 0; i < sols.size(); ++i) {
      const Pose got = forward(t, sols[i], Pose(), tool);
      worst_p = std::fmax(worst_p, pos_gap(got, target));
      worst_r = std::fmax(worst_r, rot_gap(got, target));
      if (same_angles(sols[i], q)) has = true;
    }
    if (has) ++found;
  }
  rec("poses", poses, 0);
  rec("avg_solutions", total / poses, 1);
  rec("found", found, 0);
  rec("max_err", worst_p, 10);
  rec("max_rot", worst_r, 10);
  RC_CHECK(found == poses);
  RC_CHECK(worst_p < 1e-9);
  RC_CHECK(worst_r < 1e-9);
  RC_CHECK(total / poses > 7.5);
}

RC_TEST("a pose out of reach has no solution, at the very edge the elbow up and down are one, and a straight wrist has fewer") {
  const ArmTable t = elbow_arm();
  Pose far;
  far.p[0] = 1.5;
  far.p[2] = 0.3;
  rec("far", static_cast<double>(ik_spherical(t, far, Pose(), Pose()).size()), 0);
  RC_CHECK(ik_spherical(t, far, Pose(), Pose()).empty());
  // The edge: the upper arm and the forearm in a line. The wrist centre at (a2 + d4, d1) with the flange pointing down by d6 (the tool z axis down).
  Pose edge;
  edge.r[0][0] = 1.0;
  edge.r[1][1] = -1.0;
  edge.r[2][2] = -1.0;
  // The elbow's reach when straight is a2 + d4 on the plane at the shoulder height... for this arm that is sin(theta3) = 1.
  edge.p[0] = std::sqrt(0.4 * 0.4 + 0.35 * 0.35 + 2.0 * 0.4 * 0.35) * 1.0;
  edge.p[2] = 0.3 - 0.10;
  const std::vector<std::vector<double> > e = ik_spherical(t, edge, Pose(), Pose());
  rec("edge", static_cast<double>(e.size()), 0);
  RC_CHECK(e.size() == 4);
  // A straight wrist: joint 5 at zero. For the arm configuration that made the pose the wrist has one solution and not two.
  std::vector<double> q(6, 0.0);
  q[1] = 0.3;
  q[2] = 0.6;
  q[3] = 0.2;
  q[4] = 0.0;
  q[5] = 0.5;
  const Pose sp = forward(t, q, Pose(), Pose());
  const std::vector<std::vector<double> > s = ik_spherical(t, sp, Pose(), Pose());
  rec("sing", static_cast<double>(s.size()), 0);
  RC_CHECK(s.size() >= 5 && s.size() < 8);
  for (std::size_t i = 0; i < s.size(); ++i) RC_CHECK(pos_gap(forward(t, s[i], Pose(), Pose()), sp) < 1e-9 && rot_gap(forward(t, s[i], Pose(), Pose()), sp) < 1e-9);
}

RC_TEST("the closed form is for arms whose last three axes meet, and the UR5 is not one of them") {
  RC_CHECK(has_spherical_wrist(elbow_arm()));
  const ArmTable u = ur5();
  rec("ur5_sph", has_spherical_wrist(u) ? 1 : 0, 0);
  rec("ur5_d5", u.rows[4].d, 5);
  RC_CHECK(!has_spherical_wrist(u));
  RC_CHECK(!has_spherical_wrist(panda()));
}

RC_TEST("the limits of the joints take some of the eight away") {
  ArmTable t = elbow_arm();
  for (std::size_t i = 0; i < t.rows.size(); ++i) {
    t.rows[i].lo = -2.0944;
    t.rows[i].hi = 2.0944;
  }
  const std::vector<double> q = some_q(10);
  const Pose target = forward(t, q, Pose(), Pose());
  const std::vector<std::vector<double> > all = ik_spherical(t, target, Pose(), Pose());
  const std::vector<std::vector<double> > allowed = within_limits_only(t, all);
  rec("limited", static_cast<double>(allowed.size()), 0);
  RC_CHECK(all.size() == 8);
  RC_CHECK(allowed.size() < all.size());
  RC_CHECK(!allowed.empty());
}
