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
    {"tool_felt", 7.112904688841247e-11},
    {"ur5_null", 2.3105856347706995e-07},
    {"direct_felt", 1.2437608094401749},
    {"proj_felt", 1.6395288694483426e-11},
    {"margin0", 0.08582128188313258},
    {"margin1", 0.15579703180525087},
    {"margin_plain", 0.08582128188313258},
    {"drift", 0.00036552791654449767},
    {"travel", 0.659109161781066},
    {"drift_open", 0.0007647484048702215},
    {"violation", 0.28257301672376406},
    {"clamped_q6", 0.013283288355460051},
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

std::vector<double> in_limits(const ArmTable& t, int k) {
  std::vector<double> q(t.rows.size());
  for (std::size_t i = 0; i < q.size(); ++i) {
    const double mid = 0.5 * (t.rows[i].hi + t.rows[i].lo), range = t.rows[i].hi - t.rows[i].lo;
    q[i] = mid + 0.35 * range * std::sin(1.9 * static_cast<double>(i + 1) + 0.77 * k);
  }
  return q;
}

std::vector<double> some_motion(std::size_t n, int k) {
  std::vector<double> m(n);
  for (std::size_t i = 0; i < n; ++i) m[i] = std::cos(2.3 * static_cast<double>(i + 1) + 1.1 * k);
  return m;
}

double norm(const std::vector<double>& v) {
  double s = 0.0;
  for (std::size_t i = 0; i < v.size(); ++i) s += v[i] * v[i];
  return std::sqrt(s);
}

double felt(const Jacobian& j, const std::vector<double>& m) {
  double v[6];
  tool_twist(j, m, v);
  double s = 0.0;
  for (int r = 0; r < 6; ++r) s += v[r] * v[r];
  return std::sqrt(s);
}

std::vector<double> panda_start() {
  std::vector<double> q(7);
  q[0] = 0.1;
  q[1] = -0.4;
  q[2] = 2.4;
  q[3] = -2.0;
  q[4] = 0.1;
  q[5] = 1.6;
  q[6] = 0.8;
  return q;
}

}  // namespace

RC_TEST("a joint motion put through the null space is not felt by the tool, for seven joints, and there is none for six") {
  const ArmTable p = panda(), u = ur5();
  double worst = 0.0, worst_u = 0.0;
  for (int k = 0; k < 20; ++k) {
    const Jacobian jp = jacobian(p, in_limits(p, k), Pose(), Pose(), p.convention);
    const std::vector<double> m = some_motion(7, k);
    worst = std::fmax(worst, felt(jp, null_space_motion(jp, m, 1e-6)) / norm(m));
    const Jacobian ju = jacobian(u, in_limits(u, k), Pose(), Pose(), u.convention);
    const std::vector<double> mu = some_motion(6, k);
    worst_u = std::fmax(worst_u, norm(null_space_motion(ju, mu, 1e-6)) / norm(mu));
  }
  rec("tool_felt", worst, 10);
  rec("ur5_null", worst_u, 10);
  RC_CHECK(worst < 1e-6);
  RC_CHECK(worst_u < 1e-6);
}

RC_TEST("a motion applied without the projection is felt by the tool, and through it is not") {
  const ArmTable p = panda();
  const Jacobian j = jacobian(p, in_limits(p, 3), Pose(), Pose(), p.convention);
  const std::vector<double> m = some_motion(7, 3);
  rec("direct_felt", felt(j, m), 3);
  rec("proj_felt", felt(j, null_space_motion(j, m, 1e-6)), 8);
  RC_CHECK(felt(j, m) > 0.1);
  RC_CHECK(felt(j, null_space_motion(j, m, 1e-6)) < 1e-6);
  RC_CHECK(norm(null_space_motion(j, m, 1e-6)) > 0.1);
}

RC_TEST("the elbow moves towards the middle of the limits while the hand stays, and nothing moves with no null space term") {
  const ArmTable p = panda();
  const SelfMotion a = self_motion_run(p, panda_start(), Pose(), Pose(), 40.0, 5.0, 6.0, 0.005, 1e-3);
  const SelfMotion b = self_motion_run(p, panda_start(), Pose(), Pose(), 0.0, 5.0, 6.0, 0.005, 1e-3);
  const SelfMotion c = self_motion_run(p, panda_start(), Pose(), Pose(), 40.0, 0.0, 6.0, 0.005, 1e-3);
  rec("margin0", a.margin_start, 3);
  rec("margin1", a.margin_end, 3);
  rec("margin_plain", b.margin_end, 3);
  rec("drift", a.tool_drift, 5);
  rec("travel", a.joint_travel, 2);
  rec("drift_open", c.tool_drift, 5);
  RC_CHECK(a.margin_end > a.margin_start + 0.05);
  RC_CHECK(a.tool_drift < 1e-3);
  RC_CHECK(a.joint_travel > 0.5);
  RC_CHECK(b.joint_travel < 1e-6);
  RC_CHECK_NEAR(b.margin_end, b.margin_start, 1e-6);
  RC_CHECK(c.tool_drift > 1.5 * a.tool_drift);
}

RC_TEST("clamping to the limits keeps the joints inside them, where an unclamped solution leaves one outside") {
  ArmTable p = panda();
  std::vector<double> q = panda_start();
  q[5] = -0.3;  // outside the limit of joint 6, which starts at -0.0175
  const Pose target = forward(p, q, Pose(), Pose());
  std::vector<double> from = q;
  from[5] = 0.5;
  DlsConfig free_run, clamped;
  clamped.clamp = true;
  const IkResult a = ik_dls(p, target, Pose(), Pose(), from, free_run);
  const IkResult b = ik_dls(p, target, Pose(), Pose(), from, clamped);
  const double violation = std::fmax(0.0, p.rows[5].lo - a.q[5]);
  rec("violation", violation, 3);
  rec("clamped_q6", b.q[5], 3);
  RC_CHECK(a.ok);
  RC_CHECK(!within_limits(p, a.q));
  RC_CHECK(violation > 0.05);
  RC_CHECK(within_limits(p, b.q));
}

RC_TEST("the margin and the gradient point the way the joints should go") {
  const ArmTable p = panda();
  const std::vector<double> q = panda_start();
  RC_CHECK(joint_margin(p, q) > 0.0);
  RC_CHECK_NEAR(joint_margin(p, q), (2.8973 - 2.4) / (2.0 * 2.8973), 1e-9);
  const std::vector<double> g = limit_gradient(p, q);
  // Joint 3 is above the middle of its interval, so the gradient is positive: its negative lowers it.
  RC_CHECK(g[2] > 0.0);
  std::vector<double> out = q;
  out[0] = 5.0;
  RC_CHECK(joint_margin(p, out) < 0.0);
}
