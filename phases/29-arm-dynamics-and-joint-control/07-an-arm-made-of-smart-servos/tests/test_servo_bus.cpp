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
    {"step_rad", 0.0015339807878856412},
    {"step_mm", 1.0737865515199487},
    {"cyc_fast", 2.98},
    {"cyc_slow", 14.347222222222223},
    {"rate_fast", 335.5704697986577},
    {"rate_slow", 69.6999031945789},
    {"cyc_12", 5.680000000000001},
    {"droop", 0.13325559648261343},
    {"held", 0.00020073206343462768},
    {"lag20", 96.85546515150719},
    {"lag100", 76.85289680700228},
    {"t_limit", 0.3930000000000003},
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


RC_TEST("a goal is rounded to the steps of the servo, and the step is a millimetre at the end of an arm") {
  const double step = 6.283185307179586 / 4096.0;
  rec("step_rad", step, 5);
  rec("step_mm", 1000.0 * step * 0.7, 2);
  RC_CHECK_NEAR(quantise(0.0, 4096), 0.0, 1e-15);
  RC_CHECK_NEAR(quantise(step * 3.4, 4096), step * 3.0, 1e-12);
  RC_CHECK_NEAR(quantise(step * 3.6, 4096), step * 4.0, 1e-12);
  double worst = 0.0;
  for (double a = -1.0; a < 1.0; a += 0.000731) worst = std::fmax(worst, std::fabs(quantise(a, 4096) - a));
  RC_CHECK(worst <= 0.5 * step + 1e-12);
  RC_CHECK(worst > 0.45 * step);
}

RC_TEST("the bus takes a few milliseconds a cycle and a slow bus cannot keep an arm up to date") {
  BusSpec fast, slow;
  slow.baud = 115200.0;
  const double f = bus_cycle_seconds(fast, 6), s = bus_cycle_seconds(slow, 6), f12 = bus_cycle_seconds(fast, 12);
  rec("cyc_fast", 1000.0 * f, 2);
  rec("cyc_slow", 1000.0 * s, 1);
  rec("rate_fast", 1.0 / f, 0);
  rec("rate_slow", 1.0 / s, 0);
  rec("cyc_12", 1000.0 * f12, 2);
  RC_CHECK_NEAR(bus_cycle_seconds(fast, 1), (14 + 5 + 14) * 10.0 / 1e6 + 15 * 10.0 / 1e6 + 250e-6, 1e-12);
  RC_CHECK(f12 > 1.6 * f);
  RC_CHECK(s > 4.0 * f);
  RC_CHECK(1.0 / s < 200.0);
}

RC_TEST("a load makes the servo droop by the load over its stiffness, and a shifted goal holds the pose") {
  const ServoSpec s;
  ServoJoint a(s), b(s);
  a.reset(0.5);
  b.reset(0.5);
  a.set_goal(0.5);
  b.set_goal(goal_for_load(s, 0.5, -2.0));  // a load of 2 N m pulling down
  for (int k = 0; k < 4000; ++k) {
    a.step(-2.0, 1e-3);
    b.step(-2.0, 1e-3);
  }
  rec("droop", 0.5 - a.position(), 3);
  rec("held", std::fabs(0.5 - b.position()), 4);
  RC_CHECK_NEAR(0.5 - a.position(), droop_under_load(s, 2.0), 0.002);
  RC_CHECK(std::fabs(0.5 - b.position()) < 0.002);
}

RC_TEST("a slower update is a later arrival, and a step is covered at the speed limit") {
  const ServoSpec s;
  const double l20 = ramp_lag(s, 0.5, 20.0, 4.0), l100 = ramp_lag(s, 0.5, 100.0, 4.0);
  rec("lag20", 1000.0 * l20, 0);
  rec("lag100", 1000.0 * l100, 0);
  RC_CHECK(l20 > l100 + 0.015);
  ServoJoint j(s);
  j.reset(0.0);
  j.set_goal(1.0);
  double t = 0.0;
  while (j.position() < 0.98 && t < 3.0) {
    j.step(0.0, 1e-3);
    t += 1e-3;
  }
  rec("t_limit", t, 2);
  RC_CHECK(t > 1.0 / s.max_speed);
  RC_CHECK(t < 1.0 / s.max_speed + 0.6);
}
