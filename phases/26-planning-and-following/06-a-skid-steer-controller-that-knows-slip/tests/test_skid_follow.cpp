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

const double kPi = 3.14159265358979323846;

CarPose pt(double x, double y, double th) {
  CarPose p;
  p.x = x;
  p.y = y;
  p.theta = th;
  return p;
}

// A straight of 2 m, a quarter circle to the left of radius 1.0 m, and a straight of 2 m north: from the origin, heading east.
std::vector<CarPose> bend() {
  std::vector<CarPose> v;
  for (double x = 0.0; x < 2.0; x += 0.05) v.push_back(pt(x, 0.0, 0.0));
  for (double a = 0.05; a <= kPi / 2.0 + 1e-9; a += 0.05) v.push_back(pt(2.0 + std::sin(a), 1.0 - std::cos(a), a));
  for (double y = 1.05; y <= 3.0 + 1e-9; y += 0.05) v.push_back(pt(3.0, y, kPi / 2.0));
  return v;
}

rc::field::Cell on(const rc::field::Surface& s) {
  rc::field::Cell c;
  c.surface = s;
  return c;
}

}  // namespace

RC_TEST("the wheel speeds make the body go and turn as asked") {
  rc::vehicle::SkidGeometry g;
  for (int i = 0; i < 5; ++i) {
    const double chi = 1.0 + 0.2 * i, slip = 0.02 * i;
    const double v = 0.5 + 0.1 * i, w = -0.8 + 0.4 * i;
    const SkidCommand c = skid_wheel_speeds(g, chi, slip, v, w);
    const rc::vehicle::SkidMotion m = rc::vehicle::skid_motion(g, chi, c.wl, c.wr);
    RC_CHECK_NEAR(m.w, w, 1e-9);
    RC_CHECK_NEAR(m.v * (1.0 - slip), v, 1e-9);
  }
}

RC_TEST("a controller that knows chi and slip holds the path, one that does not strays") {
  rc::vehicle::SkidGeometry g;
  const std::vector<CarPose> path = bend();
  SkidFollowConfig aware, unaware;
  aware.chi = 1.5;
  aware.slip = 0.05;
  unaware.chi = 1.0;
  unaware.slip = 0.0;
  const SkidRun a = skid_follow_run(g, 1.5, 0.05, on(rc::field::tarmac()), aware, path);
  const SkidRun u = skid_follow_run(g, 1.5, 0.05, on(rc::field::tarmac()), unaware, path);
  rec("cte_aware", a.max_cte, 3);
  rec("cte_unaware", u.max_cte, 3);
  RC_CHECK(a.reached);
  RC_CHECK(a.max_cte < 0.1);
  RC_CHECK(u.max_cte > 2.0 * a.max_cte);
}

RC_TEST("on ice the plain controller lets go in the turn and one that limits its speed does not") {
  rc::vehicle::SkidGeometry g;
  const std::vector<CarPose> path = bend();
  SkidFollowConfig plain, limited;
  plain.chi = limited.chi = 1.5;
  plain.slip = limited.slip = 0.05;
  plain.speed = limited.speed = 1.2;
  plain.limit_speed = false;
  limited.limit_speed = true;
  const SkidRun p = skid_follow_run(g, 1.5, 0.05, on(rc::field::ice()), plain, path);
  const SkidRun l = skid_follow_run(g, 1.5, 0.05, on(rc::field::ice()), limited, path);
  rec("slid_unaware", p.slid_steps, 0);
  rec("slid_aware", l.slid_steps, 0);
  rec("t_unaware", p.time, 1);
  rec("t_aware", l.time, 1);
  RC_CHECK(p.slid_steps > 0);
  RC_CHECK(l.slid_steps == 0);
  RC_CHECK(l.reached);
  RC_CHECK(l.time > p.time);
}

RC_TEST("the speed a turn allows follows the grip of the surface") {
  const double k = 1.0;
  const double vg = rc::vehicle::max_speed_for_curvature(on(rc::field::gravel()), 0.0, k);
  const double vi = rc::vehicle::max_speed_for_curvature(on(rc::field::ice()), 0.0, k);
  rec("ratio_mu", vg / vi, 2);
  RC_CHECK_NEAR(vg / vi, std::sqrt(0.45 / 0.10), 1e-9);
}

RC_TEST("a limited controller never drops below its floor speed") {
  rc::vehicle::SkidGeometry g;
  SkidFollowConfig cfg;
  cfg.chi = 1.5;
  cfg.slip = 0.05;
  cfg.limit_speed = true;
  cfg.min_speed = 0.2;
  const std::vector<CarPose> path = bend();
  // A pose facing across the path asks for a sharp turn.
  const SkidCommand c = skid_follow_command(g, on(rc::field::ice()), cfg, pt(2.0, 0.0, kPi / 2.0), path, 40);
  RC_CHECK(c.v >= 0.2 - 1e-12);
}
