#include <rc/test/rc_test.hpp>
#include <sstream>
#include <cstring>

#include <cmath>

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
    {"duty_walk", 0.75},
    {"min_walk", 3.0},
    {"duty_trot", 0.5},
    {"min_trot", 2.0},
    {"duty_run", 0.35},
    {"flight", 30.0},
    {"v", 0.4},
    {"t_st", 0.25},
    {"td_x", 0.04999999999999999},
    {"lo_x", -0.05000000000000002},
    {"v_over", 0.2},
    {"k", 0.1},
    {"extra_mm", 20.000000000000018},
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

GaitTiming walk() {
  GaitTiming g;
  g.duty = 0.75;
  g.offset[0] = 0.0;
  g.offset[1] = 0.5;
  g.offset[2] = 0.25;
  g.offset[3] = 0.75;
  return g;
}

}  // namespace

RC_TEST("a leg is in stance for the duty and in swing for the rest, and phases wrap round") {
  GaitTiming g;
  g.duty = 0.6;
  g.period = 1.0;
  RC_CHECK_NEAR(leg_phase(g, 0, 0.25), 0.25, 1e-12);
  RC_CHECK_NEAR(leg_phase(g, 0, 1.25), 0.25, 1e-12);
  RC_CHECK_NEAR(leg_phase(g, 1, 0.75), 0.25, 1e-12);   // offset 0.5
  RC_CHECK(in_stance(g, 0, 0.5) && !in_stance(g, 0, 0.7));
  int n = 0;
  for (int i = 0; i < 1000; ++i)
    if (in_stance(g, 0, i / 1000.0)) ++n;
  RC_CHECK_NEAR(n / 1000.0, 0.6, 0.002);
}

RC_TEST("a walk always has three feet down, a trot two, and a short duty has a flight") {
  GaitTiming w = walk();
  GaitTiming trot;
  GaitTiming run;
  run.duty = 0.35;
  int min_w = 4, min_t = 4, flight = 0, n = 0;
  for (int i = 0; i < 2000; ++i) {
    const double t = i * 0.5 / 2000.0 + 1e-6;
    ++n;
    const int cw = count_stance(w, t), ct = count_stance(trot, t), cr = count_stance(run, t);
    if (cw < min_w) min_w = cw;
    if (ct < min_t) min_t = ct;
    if (cr == 0) ++flight;
  }
  rec("duty_walk", w.duty, 2);
  rec("min_walk", min_w, 0);
  rec("duty_trot", trot.duty, 1);
  rec("min_trot", min_t, 0);
  rec("duty_run", run.duty, 2);
  rec("flight", 100.0 * flight / n, 0);
  RC_CHECK(min_w >= 3);
  RC_CHECK(min_t >= 2);
  RC_CHECK(flight > 0);
}

RC_TEST("a stance foot moves back in the body's frame, and the touchdown is ahead of the hip by half the stance") {
  const double v[2] = {0.4, 0.0}, v_cmd[2] = {0.4, 0.0}, hip[2] = {0.19, 0.12}, foot[2] = {0.19, 0.12};
  double fv[2];
  stance_foot_velocity(v, 0.0, foot, fv);
  RC_CHECK_NEAR(fv[0], -0.4, 1e-12);
  stance_foot_velocity(v, 0.5, foot, fv);
  RC_CHECK_NEAR(fv[0], -0.4 + 0.5 * 0.12, 1e-12);
  RC_CHECK_NEAR(fv[1], -0.5 * 0.19, 1e-12);
  const double t_st = 0.25;
  double td[2];
  touchdown_xy(hip, v, v_cmd, t_st, 0.1, td);
  rec("v", 0.4, 1);
  rec("t_st", t_st, 2);
  rec("td_x", td[0] - hip[0], 3);
  // the foot, stationary on the ground, goes back by v times the stance in the body's frame: it leaves as far behind as it arrived ahead
  const double lo = td[0] - v[0] * t_st;
  rec("lo_x", lo - hip[0], 3);
  RC_CHECK_NEAR(td[0] - hip[0], 0.05, 1e-12);
  RC_CHECK_NEAR(lo - hip[0], -0.05, 1e-12);
  RC_CHECK_NEAR(td[1], hip[1], 1e-12);
  const double fast[2] = {0.6, 0.0};
  double td2[2];
  touchdown_xy(hip, fast, v_cmd, t_st, 0.1, td2);
  rec("v_over", 0.2, 1);
  rec("k", 0.1, 1);
  rec("extra_mm", 1000.0 * (td2[0] - (hip[0] + 0.5 * fast[0] * t_st)), 0);
  RC_CHECK_NEAR(td2[0] - (hip[0] + 0.5 * fast[0] * t_st), 0.1 * 0.2, 1e-12);
  RC_CHECK(td2[0] > td[0]);
}
