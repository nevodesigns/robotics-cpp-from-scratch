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

// A foot is planned to land with its tip 1 cm above the level ground (the swing's end); the ground under it is d higher (+) or lower (-) than level. Returns the search's state when the foot touches or gives up, and the time the search took.
struct Landing {
  ReachState s;
  double t_search = 0.0;
  double touched_at = 0.0;   // the height of the foot's tip when it touched, relative to level ground
  bool early = false;        // the ground was higher than the swing's end: it touched before the swing finished
};

Landing land(double d, double rate, double max_extra) {
  Landing l;
  const double dt = 0.001;
  const double end_height = 0.01;      // the swing ends 1 cm above level ground
  if (d >= end_height) {
    l.early = true;
    l.touched_at = d;
    l.s.done = true;
    return l;
  }
  double t = 0.0;
  while (!l.s.done && !l.s.hole && t < 5.0) {
    const double tip = end_height - l.s.ext;
    reach_down_step(l.s, tip <= d, dt, rate, max_extra);
    t += dt;
  }
  l.t_search = t;
  l.touched_at = end_height - l.s.ext;
  return l;
}

}  // namespace

RC_TEST("on level ground the foot is down at the end of the swing, and a search lowers it until it touches") {
  ReachState s;
  reach_down_step(s, true, 0.001, 0.25, 0.10);
  RC_CHECK(s.done && s.ext == 0.0);
  ReachState t;
  reach_down_step(t, false, 0.01, 0.25, 0.10);
  RC_CHECK_NEAR(t.ext, 0.0025, 1e-12);
  RC_CHECK(!t.done && !t.hole);
  reach_down_step(t, true, 0.01, 0.25, 0.10);
  RC_CHECK(t.done);
  const double e = t.ext;
  reach_down_step(t, false, 0.01, 0.25, 0.10);   // it stays where it was found
  RC_CHECK_NEAR(t.ext, e, 1e-12);
}

RC_TEST("a bump is met early, a step down is found by reaching, and a hole is given up on") {
  const double rate = 0.25, max_extra = 0.10;
  const Landing flat = land(0.0, rate, max_extra), bump = land(0.03, rate, max_extra), step = land(-0.05, rate, max_extra), hole = land(-0.15, rate, max_extra);
  rec("ext_flat", flat.s.ext, 3);
  rec("t_flat", flat.t_search, 2);
  rec("bump_cm", 3, 0);
  rec("ext_bump", bump.s.ext, 3);
  rec("step_cm", 5, 0);
  rec("ext_step", step.s.ext, 3);
  rec("rate", rate, 2);
  rec("t_step", step.t_search, 2);
  rec("hole_cm", 15, 0);
  rec("max_cm", max_extra * 100.0, 0);
  rec("t_hole", hole.t_search, 2);
  RC_CHECK(bump.early && bump.s.ext == 0.0);
  RC_CHECK(step.s.done && !step.s.hole);
  RC_CHECK_NEAR(step.s.ext, 0.06, 0.002);          // 1 cm above level plus the 5 cm of the step
  RC_CHECK_NEAR(step.t_search, 0.06 / rate, 0.01);
  RC_CHECK(hole.s.hole && !hole.s.done);
  RC_CHECK_NEAR(hole.s.ext, max_extra, 1e-12);
  RC_CHECK(flat.s.done || flat.s.ext > 0.0);
  RC_CHECK_NEAR(flat.touched_at, 0.0, 0.002);
}

RC_TEST("a gain keeps one bump from moving the estimate much, and the body follows the mean of the feet that are down") {
  const double gain = 0.2, bump = 0.03;
  double est = 0.0;
  est = update_ground(est, bump, gain);
  const double e1 = est;
  for (int i = 0; i < 9; ++i) est = update_ground(est, bump, gain);
  rec("gain", gain, 1);
  rec("est1_mm", e1 * 1000.0, 1);
  rec("est10_mm", est * 1000.0, 1);
  RC_CHECK_NEAR(e1, 0.006, 1e-12);
  RC_CHECK(est > 0.8 * bump && est < bump);
  const double ests[4] = {0.03, 0.03, 0.0, 0.0};
  const bool all[4] = {true, true, true, true}, front[4] = {true, true, false, false};
  rec("body_mm", 1000.0 * (body_height_from(0.28, ests, all) - 0.28), 0);
  RC_CHECK_NEAR(body_height_from(0.28, ests, all), 0.28 + 0.015, 1e-12);
  RC_CHECK_NEAR(body_height_from(0.28, ests, front), 0.28 + 0.03, 1e-12);
  const bool none[4] = {false, false, false, false};
  RC_CHECK_NEAR(body_height_from(0.28, ests, none), 0.28, 1e-12);
}
