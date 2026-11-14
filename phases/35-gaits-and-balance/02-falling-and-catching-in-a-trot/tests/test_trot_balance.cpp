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
    {"h", 0.27},
    {"tc", 0.1659003790827993},
    {"x0_mm", 10.0},
    {"L_cm", 6.0},
    {"t_small", 0.4110826796798543},
    {"x1_mm", 30.0},
    {"t_big", 0.2184838143359132},
    {"v0", 0.1},
    {"t_moving", 0.14924179245788893},
    {"h_hi", 0.4},
    {"t_taller", 0.5003539036896151},
    {"t_stance", 0.25},
    {"ok_small", 1.0},
    {"ok_big", 0.0},
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

// The same fall by integrating x'' = (g / h) x with small steps.
double numeric_time(double x0, double v0, double L, double h) {
  const double k = 9.81 / h, dt = 1e-5;
  double x = x0, v = v0, t = 0.0;
  while (x < L && t < 5.0) {
    v += k * x * dt;
    x += v * dt;
    t += dt;
  }
  return x >= L ? t : -1.0;
}

}  // namespace

RC_TEST("the closed form agrees with integrating the pendulum, and a body that starts at the limit has no time") {
  const double h = 0.27;
  const double x0s[3] = {0.005, 0.02, 0.04}, v0s[3] = {0.0, 0.1, 0.0};
  for (int i = 0; i < 3; ++i) {
    const double a = time_to_move(x0s[i], v0s[i], 0.06, h), b = numeric_time(x0s[i], v0s[i], 0.06, h);
    RC_CHECK_NEAR(a, b, 2e-3);
  }
  RC_CHECK_NEAR(time_to_move(0.06, 0.0, 0.06, h), 0.0, 1e-12);
  RC_CHECK(time_to_move(0.01, -1.0, 0.06, h) < 0.0);   // moving back toward the line fast: never gets there
  RC_CHECK_NEAR(pendulum_time(h), std::sqrt(h / 9.81), 1e-12);
}

RC_TEST("a body falls away from the line of its feet sooner the further off it starts, and later the taller it is") {
  const double h = 0.27, L = 0.06;
  const double small = time_to_move(0.010, 0.0, L, h), big = time_to_move(0.030, 0.0, L, h);
  const double moving = time_to_move(0.030, 0.10, L, h);
  const double taller = time_to_move(0.010, 0.0, L, 0.40);
  rec("h", h, 2);
  rec("tc", pendulum_time(h), 3);
  rec("x0_mm", 10, 0);
  rec("L_cm", 6, 0);
  rec("t_small", small, 2);
  rec("x1_mm", 30, 0);
  rec("t_big", big, 2);
  rec("v0", 0.10, 2);
  rec("t_moving", moving, 2);
  rec("h_hi", 0.40, 2);
  rec("t_taller", taller, 2);
  RC_CHECK(small > big);
  RC_CHECK(moving < big);
  RC_CHECK(taller > small);
  RC_CHECK(small > 0.3);
}

RC_TEST("a stance is long enough or too long for the body's offset, with a margin") {
  const double h = 0.27, L = 0.06, stance = 0.25;
  const bool ok_small = trot_stance_ok(stance, 0.010, 0.0, L, h, 0.7);
  const bool ok_big = trot_stance_ok(stance, 0.030, 0.0, L, h, 0.7);
  rec("t_stance", stance, 2);
  rec("ok_small", ok_small ? 1 : 0, 0);
  rec("ok_big", ok_big ? 1 : 0, 0);
  RC_CHECK(ok_small);
  RC_CHECK(!ok_big);
  RC_CHECK(!trot_stance_ok(stance, 0.010, -1.0, L, h, 0.7));
  RC_CHECK(trot_stance_ok(0.05, 0.030, 0.0, L, h, 0.7));
}
