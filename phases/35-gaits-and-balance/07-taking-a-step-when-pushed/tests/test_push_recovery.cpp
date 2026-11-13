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


RC_TEST("the capture point is where the body would come to rest, and grows exponentially if nothing is done") {
  const double h = 0.27, T = time_constant(h);
  RC_CHECK_NEAR(T, std::sqrt(h / 9.81), 1e-12);
  RC_CHECK_NEAR(capture_point(0.02, 0.3, h), 0.02 + 0.3 * T, 1e-12);
  RC_CHECK_NEAR(capture_at(0.05, 0.0, h), 0.05, 1e-12);
  RC_CHECK_NEAR(capture_at(0.05, T, h), 0.05 * std::exp(1.0), 1e-12);
  // the pendulum itself: the capture point x + v T of x(t) = x0 cosh(t / T) + v0 T sinh(t / T) is x0 + v0 T times e^(t / T)
  const double x0 = 0.02, v0 = 0.3, t = 0.2;
  const double x = x0 * std::cosh(t / T) + v0 * T * std::sinh(t / T);
  const double v = x0 / T * std::sinh(t / T) + v0 * std::cosh(t / T);
  RC_CHECK_NEAR(capture_point(x, v, h), capture_at(capture_point(x0, v0, h), t, h), 1e-9);
}

RC_TEST("the foot must land beyond the capture point of the landing time, and the push that can be answered falls with the swing time") {
  const double h = 0.27, reach = 0.25, margin = 0.02;
  const double dv = 0.30, t_sw = 0.25;
  const double xi0 = capture_point(0.0, dv, h);
  const StepPlan p = step_target(xi0, t_sw, h, reach, margin);
  rec("h", h, 2);
  rec("tc", time_constant(h), 3);
  rec("dv", dv, 2);
  rec("xi0_cm", xi0 * 100.0, 1);
  rec("t_sw", t_sw, 2);
  rec("target_cm", p.target * 100.0, 1);
  rec("ok_a", p.ok ? 1 : 0, 0);
  rec("reach_cm", reach * 100.0, 0);
  RC_CHECK_NEAR(p.xi_land, xi0 * std::exp(t_sw / time_constant(h)), 1e-12);
  RC_CHECK_NEAR(p.target, p.xi_land + margin, 1e-12);
  const double t[3] = {0.15, 0.25, 0.35};
  double v[3];
  for (int i = 0; i < 3; ++i) v[i] = max_push_speed(reach, margin, t[i], h);
  rec("t1", t[0], 2);
  rec("t2", t[1], 2);
  rec("t3", t[2], 2);
  rec("v1", v[0], 2);
  rec("v2", v[1], 2);
  rec("v3", v[2], 2);
  RC_CHECK(v[0] > v[1] && v[1] > v[2]);
  // at that speed the step just reaches; a little more does not
  for (int i = 0; i < 3; ++i) {
    const double xi_at = capture_point(0.0, v[i], h);
    RC_CHECK(step_target(xi_at, t[i], h, reach, margin).ok);
    RC_CHECK(!step_target(capture_point(0.0, v[i] * 1.05, h), t[i], h, reach, margin).ok);
  }
}

RC_TEST("a foot landed at the target leaves the capture point inside it, and one landed short leaves it outside") {
  const double h = 0.27, T = time_constant(h), margin = 0.02, t_land = 0.25;
  const double x0 = 0.0, v0 = 0.30;
  // the body's place and speed when the foot lands
  const double x = x0 * std::cosh(t_land / T) + v0 * T * std::sinh(t_land / T);
  const double v = x0 / T * std::sinh(t_land / T) + v0 * std::cosh(t_land / T);
  const StepPlan p = step_target(capture_point(x0, v0, h), t_land, h, 1.0, margin);
  const double rel = capture_point(x, v, h) - p.target;
  rec("rel_cm", -rel * 100.0, 1);
  RC_CHECK_NEAR(rel, -margin, 1e-9);
  const double short_target = 0.8 * p.target;
  RC_CHECK(capture_point(x, v, h) - short_target > 0.0);
}
