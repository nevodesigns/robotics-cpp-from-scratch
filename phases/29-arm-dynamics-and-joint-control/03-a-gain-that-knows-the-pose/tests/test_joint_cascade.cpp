#include <rc/test/rc_test.hpp>

#include <cmath>
#include <iostream>

#include "rc/arm/ik_closed.hpp"
#include "solution.hpp"
#include <iomanip>
#include <iostream>

static void rec(const char* key, double value, int digits = 3) {
  std::cout << "@" << key << " " << std::fixed << std::setprecision(digits) << value << std::endl;
  std::cout.unsetf(std::ios::floatfield);
}
[[maybe_unused]] static void recs(const char* key, const std::string& value) { std::cout << "@" << key << " " << value << std::endl; }


namespace {

ArmModel typical() {
  ArmModel m;
  m.table = elbow_arm();
  std::vector<double> masses(6);
  masses[0] = 3.0;
  masses[1] = 4.0;
  masses[2] = 2.0;
  masses[3] = 2.0;
  masses[4] = 1.0;
  masses[5] = 0.5;
  m.links = rod_links(m.table, masses, 0.04);
  return m;
}

// The lightest and heaviest the shoulder sees over a spread of poses.
void shoulder_range(const ArmModel& m, double rotor, double gear, double& lo, double& hi) {
  lo = 1e9;
  hi = 0.0;
  for (int k = 0; k < 60; ++k) {
    std::vector<double> q(6);
    for (int i = 0; i < 6; ++i) q[static_cast<std::size_t>(i)] = 1.4 * std::sin(2.3 * (i + 1) + 0.9 * k);
    const double j = effective_inertia(m, q, 1, rotor, gear);
    lo = std::fmin(lo, j);
    hi = std::fmax(hi, j);
  }
}

}  // namespace

RC_TEST("the inertia the shoulder sees changes several fold over the poses, and a gear makes it change less") {
  const ArmModel m = typical();
  double lo, hi, lo_g, hi_g;
  shoulder_range(m, 0.0, 1.0, lo, hi);
  shoulder_range(m, 1e-4, 100.0, lo_g, hi_g);
  rec("j_min", lo, 3);
  rec("j_max", hi, 3);
  rec("ratio", hi / lo, 1);
  rec("ratio_geared", hi_g / lo_g, 1);
  RC_CHECK(hi / lo > 2.5);
  RC_CHECK(hi_g / lo_g < 0.7 * hi / lo);
  RC_CHECK(lo > 0.0);
}

RC_TEST("gains tuned for the heaviest pose are good there and ring at the lightest; gains that follow the inertia are good at both") {
  const ArmModel m = typical();
  double lo, hi;
  shoulder_range(m, 0.0, 1.0, lo, hi);
  const double wp = 15.0, wv = 80.0;
  LoopConfig c;
  c.delay = 3;
  const CascadeGains heavy = scheduled_gains(hi, wp, wv);
  const StepResult at_heavy = step_response(hi, heavy, 0.5, c);
  const StepResult at_light = step_response(lo, heavy, 0.5, c);
  const StepResult s_light = step_response(lo, scheduled_gains(lo, wp, wv), 0.5, c);
  const StepResult s_heavy = step_response(hi, scheduled_gains(hi, wp, wv), 0.5, c);
  rec("os_heavy", at_heavy.overshoot_percent, 1);
  rec("os_light", at_light.overshoot_percent, 0);
  rec("os_sched_light", s_light.overshoot_percent, 1);
  rec("os_sched_heavy", s_heavy.overshoot_percent, 1);
  RC_CHECK(at_heavy.stable && s_light.stable && s_heavy.stable);
  RC_CHECK(at_heavy.overshoot_percent < 15.0);
  RC_CHECK(!at_light.stable || at_light.overshoot_percent > 3.0 * at_heavy.overshoot_percent + 10.0);
  RC_CHECK(s_light.overshoot_percent < 15.0 && s_heavy.overshoot_percent < 15.0);
}

RC_TEST("a step into the limit overshoots far more if the integral keeps running") {
  const ArmModel m = typical();
  double lo, hi;
  shoulder_range(m, 0.0, 1.0, lo, hi);
  LoopConfig on, held;
  on.tau_max = held.tau_max = 20.0;
  on.anti_windup = false;
  held.anti_windup = true;
  on.seconds = held.seconds = 4.0;
  const CascadeGains g = scheduled_gains(hi, 15.0, 80.0);
  const StepResult a = step_response(hi, g, 1.5, on), b = step_response(hi, g, 1.5, held);
  rec("os_windup", a.overshoot_percent, 0);
  rec("os_aw", b.overshoot_percent, 1);
  RC_CHECK(b.max_torque <= 20.0 + 1e-9);
  RC_CHECK(a.overshoot_percent > b.overshoot_percent + 5.0);
}

RC_TEST("the gains scale with the inertia") {
  const CascadeGains a = scheduled_gains(0.5, 10.0, 100.0), b = scheduled_gains(1.5, 10.0, 100.0);
  RC_CHECK_NEAR(b.kp_vel, 3.0 * a.kp_vel, 1e-12);
  RC_CHECK_NEAR(b.ki_vel, 3.0 * a.ki_vel, 1e-12);
  RC_CHECK_NEAR(a.kp_pos, b.kp_pos, 1e-12);
  RC_CHECK_NEAR(a.kp_vel, 0.5 * 100.0, 1e-12);
}
