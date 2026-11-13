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


RC_TEST("the speed a person can be allowed falls with the delay, and it does stop within the sight line") {
  const double sight = 6.0, a = 2.0;
  const double v05 = operator_speed_limit(sight, 0.2, 0.2, 0.1, a);
  const double v10 = operator_speed_limit(sight, 0.4, 0.4, 0.2, a);
  const double v20 = operator_speed_limit(sight, 0.8, 0.8, 0.4, a);
  rec("v_d05", v05, 2);
  rec("v_d10", v10, 2);
  rec("v_d20", v20, 2);
  RC_CHECK(v05 > v10 && v10 > v20);
  // Step a vehicle: it goes on at v for the delay, then brakes at a.
  const double dt = 0.0005;
  const double T[3] = {0.5, 1.0, 2.0};
  const double V[3] = {v05, v10, v20};
  for (int k = 0; k < 3; ++k) {
    double x = 0.0, v = V[k], t = 0.0;
    while (t < T[k]) {
      x += v * dt;
      t += dt;
    }
    while (v > 0.0) {
      x += v * dt;
      v -= a * dt;
    }
    RC_CHECK_NEAR(x, sight, 0.02);
  }
}

RC_TEST("when the commands stop the vehicle stops, after the timeout and then the braking") {
  const double timeouts[2] = {0.5, 2.0};
  double dist[2] = {0.0, 0.0};
  for (int k = 0; k < 2; ++k) {
    TeleopLimits lim;
    lim.timeout = timeouts[k];
    TeleopGovernor g(lim);
    const double dt = 0.01;
    double x = 0.0, t = 0.0, x_silence = 0.0;
    for (; t < 6.0; t += dt) {
      g.request(t, 2.0);
      x += g.step(t, dt, -1.0) * dt;
    }
    x_silence = x;
    for (; t < 30.0; t += dt) x += g.step(t, dt, -1.0) * dt;  // no more requests
    dist[k] = x - x_silence;
    RC_CHECK(g.speed() == 0.0);
  }
  rec("dist_silence", dist[0], 2);
  rec("dist_silence_long", dist[1], 2);
  // v * timeout + v^2 / (2 decel): 2 * 0.5 + 1.0, and 2 * 2.0 + 1.0.
  RC_CHECK_NEAR(dist[0], 2.0, 0.1);
  RC_CHECK_NEAR(dist[1], 5.0, 0.1);
}

RC_TEST("a request is reached by ramping, not at once") {
  TeleopLimits lim;
  TeleopGovernor g(lim);
  const double dt = 0.01;
  double t = 0.0;
  for (; t < 5.0; t += dt) {
    g.request(t, 2.0);
    if (g.step(t, dt, -1.0) >= 2.0 - 1e-9) break;
  }
  rec("t_ramp", t, 2);
  RC_CHECK_NEAR(t, 2.0 / 1.5, 0.05);
}

RC_TEST("an obstacle the vehicle senses stops it short whatever the operator asks, and without the cap it goes through") {
  TeleopLimits lim;
  const double dt = 0.01, obstacle_x = 20.0;
  double gap = 0.0, overrun = 0.0;
  {
    TeleopGovernor g(lim);
    double x = 0.0;
    for (double t = 0.0; t < 40.0; t += dt) {
      g.request(t, 2.0);
      x += g.step(t, dt, obstacle_x - x) * dt;
    }
    gap = obstacle_x - x;
  }
  {
    TeleopGovernor g(lim);
    double x = 0.0;
    for (double t = 0.0; t < 40.0; t += dt) {
      g.request(t, 2.0);
      x += g.step(t, dt, -1.0) * dt;
    }
    overrun = x - obstacle_x;
  }
  rec("gap_shared", gap, 2);
  rec("overrun", overrun, 1);
  RC_CHECK(gap > 0.1);
  RC_CHECK(gap < lim.margin + 0.5);
  RC_CHECK(overrun > 10.0);
}

RC_TEST("a vehicle that has heard nothing stays still") {
  TeleopLimits lim;
  TeleopGovernor g(lim);
  for (double t = 0.0; t < 5.0; t += 0.01) RC_CHECK(g.step(t, 0.01, -1.0) == 0.0);
}
