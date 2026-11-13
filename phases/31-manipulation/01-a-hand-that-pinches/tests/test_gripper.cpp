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


RC_TEST("the force that holds a mass is the weight over the friction of two pads, and grows with the acceleration") {
  Gripper g;
  const double f_rest = required_force(g, 1.0, 0.0, 1.0);
  const double f_lift = required_force(g, 1.0, 5.0, 1.0);
  rec("stroke", g.stroke, 3);
  rec("fmax", g.f_max, 0);
  rec("m_hold", 1.0, 0);
  rec("f_rest", f_rest, 2);
  rec("mu", g.mu, 1);
  rec("acc", 5.0, 0);
  rec("f_lift", f_lift, 2);
  RC_CHECK_NEAR(f_rest, 9.81, 1e-9);
  RC_CHECK_NEAR(f_lift, 14.81, 1e-9);
  RC_CHECK_NEAR(required_force(g, 2.0, 0.0, 1.0), 2.0 * f_rest, 1e-9);
  RC_CHECK_NEAR(required_force(g, 1.0, 0.0, 2.0), 2.0 * f_rest, 1e-9);
  Gripper slick = g;
  slick.mu = 0.25;
  RC_CHECK_NEAR(required_force(slick, 1.0, 0.0, 1.0), 2.0 * f_rest, 1e-9);
}

RC_TEST("the heaviest payload is the same sum turned round, and halving the friction halves it") {
  Gripper g;
  const double dry = max_payload(g, g.f_max, 0.0, 1.5);
  Gripper wet = g;
  wet.mu = 0.5 * g.mu;
  const double w = max_payload(wet, g.f_max, 0.0, 1.5);
  rec("pay_dry", dry, 1);
  rec("pay_wet", w, 1);
  RC_CHECK_NEAR(w, 0.5 * dry, 1e-9);
  RC_CHECK_NEAR(required_force(g, dry, 0.0, 1.5), g.f_max, 1e-9);
  RC_CHECK(dry > 10.0);
}

RC_TEST("a width fits if it is between closed and open, and closing takes the distance over the finger speed") {
  Gripper g;
  RC_CHECK(fits(g, 0.04));
  RC_CHECK(!fits(g, 0.09));
  RC_CHECK(!fits(g, -0.01));
  RC_CHECK_NEAR(close_time(g, 0.035), 0.05 / 0.15, 1e-9);
  RC_CHECK(close_time(g, 0.09) < 0.0);
  RC_CHECK_NEAR(grip_margin(40.0, 20.0), 2.0, 1e-12);
  RC_CHECK(grip_margin(40.0, 0.0) > 1e6);
}
