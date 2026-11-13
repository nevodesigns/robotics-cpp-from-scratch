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

const double kDeg = 3.14159265358979323846 / 180.0;

}  // namespace

RC_TEST("the time to the ground follows the pendulum, falls with the tilt and rate, and is the cap for a body that rights itself") {
  const double h = 0.27, T = std::sqrt(h / 9.81);
  // for small angles theta(t) = theta0 cosh(t / T) + omega0 T sinh(t / T): the time to reach 0.2 rad is known in closed form
  const double th0 = 0.05, w0 = 0.0, target = 0.2;
  const double analytic = T * std::acosh(target / th0);
  RC_CHECK_NEAR(time_to_ground(th0, w0, h, 5.0) > 0.0 ? 1.0 : 0.0, 1.0, 1e-12);
  const double t_small_angle = time_to_ground(0.05, 0.0, h, 5.0);
  RC_CHECK(t_small_angle > 4.0 * T && t_small_angle < 2.0 * analytic + 1.0);
  RC_CHECK(time_to_ground(0.4, 3.0, h, 5.0) < time_to_ground(0.3, 0.2, h, 5.0));
  RC_CHECK(time_to_ground(0.3, 3.0, h, 5.0) < time_to_ground(0.3, 0.0, h, 5.0));
  RC_CHECK_NEAR(time_to_ground(0.2, -3.0, h, 5.0), 5.0, 1e-12);   // swinging back to upright fast enough: it never falls (it rights itself)
  RC_CHECK_NEAR(time_to_ground(-0.4, -3.0, h, 5.0), time_to_ground(0.4, 3.0, h, 5.0), 1e-9);   // the same either side
}

RC_TEST("a quick fall has no time for a step and goes soft, a slow one is recovered, and a small tilt is left alone") {
  const double h = 0.27, t_need = 0.30, warn = 0.05, warn_rate = 0.5, fallen = 1.2;
  const double th0 = 0.4, w0 = 3.0, th1 = 0.3, w1 = 0.2;
  const double t0 = time_to_ground(th0, w0, h, 5.0), t1 = time_to_ground(th1, w1, h, 5.0);
  const int a0 = fall_action(th0, w0, h, t_need, warn, warn_rate, fallen), a1 = fall_action(th1, w1, h, t_need, warn, warn_rate, fallen);
  rec("h", h, 2);
  rec("th0_deg", th0 / kDeg, 0);
  rec("w0", w0, 1);
  rec("t_fall", t0, 2);
  rec("th1_deg", th1 / kDeg, 0);
  rec("w1", w1, 1);
  rec("t_slow", t1, 2);
  rec("t_need", t_need, 2);
  rec("act_fast", a0, 0);
  rec("act_slow", a1, 0);
  RC_CHECK(t0 < t_need && t1 > t_need);
  RC_CHECK(a0 == kFallSoft && a1 == kFallRecover);
  RC_CHECK(fall_action(0.01, 0.0, h, t_need, warn, warn_rate, fallen) == kFallNone);
  RC_CHECK(fall_action(1.3, 0.5, h, t_need, warn, warn_rate, fallen) == kFallen);
  RC_CHECK(fall_action(-0.4, -3.0, h, t_need, warn, warn_rate, fallen) == kFallSoft);
}

RC_TEST("a landing is softer the longer the stroke over which it is taken up") {
  const double mass = 12.0, drop = 0.19, stiff = 0.02, soft = 0.10;
  const double fs = impact_force(mass, drop, stiff), fl = impact_force(mass, drop, soft);
  rec("mass", mass, 0);
  rec("drop", drop, 2);
  rec("f_stiff", fs, 0);
  rec("stroke_stiff", stiff, 2);
  rec("f_soft", fl, 0);
  rec("stroke_soft", soft, 2);
  rec("ratio", fs / fl, 0);
  RC_CHECK_NEAR(fs * stiff, mass * 9.81 * drop, 1e-9);
  RC_CHECK_NEAR(fs / fl, soft / stiff, 1e-9);
}
