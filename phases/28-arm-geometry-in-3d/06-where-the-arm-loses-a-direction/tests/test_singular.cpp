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
    {"sv_generic", 0.17188422633730896},
    {"sv_wrist", 0.0},
    {"sv_elbow", 0.0},
    {"sv_shoulder", 2.0454652946487363e-09},
    {"sv_5", 0.1336588988963615},
    {"sv_1", 0.031273811445359505},
    {"sv_01", 0.0031744231181125354},
    {"near", 0.01},
    {"peak_plain", 26.079191426944345},
    {"peak_damped", 2.036616256663527},
    {"err_plain", 0.0005709774149424683},
    {"err_damped", 0.044792325875554526},
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

std::vector<double> generic() {
  std::vector<double> q(6);
  q[0] = 0.4;
  q[1] = 0.7;
  q[2] = 0.3;
  q[3] = 0.5;
  q[4] = 0.9;
  q[5] = -0.4;
  return q;
}

double smallest_at(const ArmTable& t, const std::vector<double>& q) { return smallest_singular(jacobian(t, q, Pose(), Pose(), t.convention)); }

}  // namespace

RC_TEST("the singular values of a Jacobian that is a diagonal matrix are its diagonal, and the product is its determinant") {
  Jacobian j;
  j.n = 6;
  const double d[6] = {0.1, 3.0, 0.5, 2.0, 0.25, 1.0};
  for (int i = 0; i < 6; ++i) j.m[i][i] = d[i];
  double s[6];
  singular_values(j, s);
  const double want[6] = {3.0, 2.0, 1.0, 0.5, 0.25, 0.1};
  for (int i = 0; i < 6; ++i) RC_CHECK_NEAR(s[i], want[i], 1e-12);
  RC_CHECK_NEAR(manipulability(j), 3.0 * 2.0 * 1.0 * 0.5 * 0.25 * 0.1, 1e-12);
  RC_CHECK_NEAR(condition_number(j), 30.0, 1e-9);
}

RC_TEST("the three kinds of singularity of an arm with a spherical wrist each take the smallest singular value to zero") {
  const ArmTable t = elbow_arm();
  std::vector<double> q = generic();
  const double generic_sv = smallest_at(t, q);
  rec("sv_generic", generic_sv, 3);
  std::vector<double> w = q;
  w[4] = 0.0;
  rec("sv_wrist", smallest_at(t, w), 8);
  std::vector<double> e = q;
  e[2] = kPi / 2.0;  // sin(theta3) = 1: the elbow stretched
  rec("sv_elbow", smallest_at(t, e), 8);
  // The wrist centre over the shoulder axis: X = a2 cos(theta2) + d4 sin(theta2 + theta3) = 0.
  std::vector<double> s = q;
  s[1] = 1.0;
  s[2] = std::asin(-(0.4 / 0.35) * std::cos(1.0)) - 1.0;
  rec("sv_shoulder", smallest_at(t, s), 8);
  RC_CHECK(generic_sv > 0.05);
  RC_CHECK(smallest_at(t, w) < 1e-6);  // a square root halves the digits: an eigenvalue of 1e-16 is a singular value of 1e-8
  RC_CHECK(smallest_at(t, e) < 1e-6);
  RC_CHECK(smallest_at(t, s) < 1e-6);
}

RC_TEST("the smallest singular value falls as the wrist straightens") {
  const ArmTable t = elbow_arm();
  std::vector<double> q = generic();
  q[4] = 0.5;
  const double a = smallest_at(t, q);
  q[4] = 0.1;
  const double b = smallest_at(t, q);
  q[4] = 0.01;
  const double c = smallest_at(t, q);
  rec("sv_5", a, 4);
  rec("sv_1", b, 4);
  rec("sv_01", c, 5);
  RC_CHECK(a > b && b > c);
  RC_CHECK(c < 0.2 * a);
}

RC_TEST("the damping is none well away, and rises to its maximum at the singularity") {
  RC_CHECK_NEAR(adaptive_lambda(0.5, 0.1, 0.1), 0.0, 1e-15);
  RC_CHECK_NEAR(adaptive_lambda(0.1, 0.1, 0.1), 0.0, 1e-15);
  RC_CHECK_NEAR(adaptive_lambda(0.0, 0.1, 0.1), 0.1, 1e-15);
  RC_CHECK(adaptive_lambda(0.05, 0.1, 0.1) > 0.0 && adaptive_lambda(0.05, 0.1, 0.1) < 0.1);
  RC_CHECK(adaptive_lambda(0.02, 0.1, 0.1) > adaptive_lambda(0.05, 0.1, 0.1));
}

RC_TEST("drawing a line past the shoulder axis, an undamped step turns joint 1 hard and a damped one does not, at a price in accuracy") {
  const ArmTable t = elbow_arm();
  const double near = 0.01;
  Pose start;
  start.r[1][1] = -1.0;
  start.r[2][2] = -1.0;
  start.p[0] = -0.35;
  start.p[1] = near;
  start.p[2] = 0.35;
  const std::vector<std::vector<double> > sols = ik_spherical(t, start, Pose(), Pose());
  RC_CHECK(!sols.empty());
  std::vector<double> q0 = sols[0];
  double best = 1e9;
  for (std::size_t i = 0; i < sols.size(); ++i) {
    double n = 0.0;
    for (int k = 0; k < 6; ++k) n += sols[i][k] * sols[i][k];
    if (n < best) {
      best = n;
      q0 = sols[i];
    }
  }
  const double dir[3] = {1.0, 0.0, 0.0};
  RateConfig plain, damped;
  damped.adaptive = true;
  const RateRun a = resolved_rate_line(t, q0, Pose(), Pose(), start.p, dir, 0.25, 0.7, plain);
  const RateRun b = resolved_rate_line(t, q0, Pose(), Pose(), start.p, dir, 0.25, 0.7, damped);
  rec("near", near, 2);
  rec("peak_plain", a.peak_rate, 1);
  rec("peak_damped", b.peak_rate, 1);
  rec("err_plain", a.max_off_line, 4);
  rec("err_damped", b.max_off_line, 4);
  RC_CHECK(a.peak_rate > 2.0 * b.peak_rate);
  RC_CHECK(b.max_off_line > a.max_off_line);
}
