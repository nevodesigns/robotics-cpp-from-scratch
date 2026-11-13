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

// A robot moves toward a person at v_nom along a line, and brakes when the separation falls to its protective distance (after the reaction time); the person keeps walking at v_h until the robot is at rest. Returns the separation at rest.
double rest_separation(const SsmParams& believed, const SsmParams& truth, double v_nom, double d0) {
  const double dt = 0.0005;
  double xr = 0.0, xh = d0, v = v_nom;
  double trigger_t = -1.0;
  for (int i = 0; i < 200000; ++i) {
    const double t = i * dt;
    const double s = xh - xr;
    if (trigger_t < 0.0 && s <= protective_distance(believed, v)) trigger_t = t;
    if (trigger_t >= 0.0 && t >= trigger_t + truth.t_r) {
      v -= truth.decel * dt;
      if (v < 0.0) v = 0.0;
    }
    xr += v * dt;
    xh -= truth.v_h * dt;
    if (trigger_t >= 0.0 && v <= 0.0) return xh - xr;
  }
  return xh - xr;
}

}  // namespace

RC_TEST("the protective distance is the sum of its terms, and grows with the robot's speed") {
  SsmParams p;
  const double vr = 1.0;
  const double sp = protective_distance(p, vr);
  const double sp0 = protective_distance(p, 0.0);
  rec("vh", p.v_h, 1);
  rec("tr_ms", p.t_r * 1000.0, 0);
  rec("dec", p.decel, 0);
  rec("k_cm", (p.c + p.z_d + p.z_r) * 100.0, 0);
  rec("vr", vr, 1);
  rec("sp", sp, 2);
  rec("sp0", sp0, 2);
  RC_CHECK_NEAR(sp, 1.6 * (0.1 + 0.2) + 1.0 * 0.1 + 0.1 + 0.17, 1e-12);
  RC_CHECK_NEAR(sp0, 1.6 * 0.1 + 0.17, 1e-12);
  RC_CHECK(protective_distance(p, 2.0) > sp);
}

RC_TEST("the greatest speed a separation allows is the inverse, and is zero when standing still does not fit") {
  SsmParams p;
  const double s1 = 1.0, s2 = 2.0;
  const double v1 = max_speed(p, s1), v2 = max_speed(p, s2);
  rec("sep1", s1, 1);
  rec("sep2", s2, 1);
  rec("v1", v1, 2);
  rec("v2", v2, 2);
  RC_CHECK_NEAR(protective_distance(p, v1), s1, 1e-9);
  RC_CHECK_NEAR(protective_distance(p, v2), s2, 1e-9);
  RC_CHECK(v2 > v1 && v1 > 0.0);
  RC_CHECK_NEAR(max_speed(p, protective_distance(p, 0.0) - 0.01), 0.0, 1e-12);
}

RC_TEST("a robot that brakes at the protective distance stops before the person, and one that leaves out a term does not") {
  SsmParams truth;
  SsmParams nohuman = truth;
  nohuman.v_h = 0.0;
  SsmParams notr = truth;
  notr.t_r = 0.0;
  const double ok = rest_separation(truth, truth, 1.0, 4.0);
  const double nh = rest_separation(nohuman, truth, 1.0, 4.0);
  const double nt = rest_separation(notr, truth, 1.0, 4.0);
  rec("rest_ok", ok, 2);
  rec("rest_nohuman", nh, 2);
  rec("rest_notr", nt, 2);
  RC_CHECK(ok >= (truth.c + truth.z_d + truth.z_r) - 0.01);
  RC_CHECK(nh < 0.0);
  RC_CHECK(nt < ok);
}
