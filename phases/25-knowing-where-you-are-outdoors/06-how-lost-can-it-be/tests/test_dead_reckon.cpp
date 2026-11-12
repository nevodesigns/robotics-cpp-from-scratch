#include <rc/test/rc_test.hpp>

#include <algorithm>
#include <cmath>
#include <iostream>
#include <vector>

#include <rc/core/random.hpp>

#include "solution.hpp"
#include <iomanip>
#include <iostream>

static void rec(const char* key, double value, int digits = 3) {
  std::cout << "@" << key << " " << std::fixed << std::setprecision(digits) << value << std::endl;
  std::cout.unsetf(std::ios::floatfield);
}
[[maybe_unused]] static void recs(const char* key, const std::string& value) { std::cout << "@" << key << " " << value << std::endl; }


namespace {

// The bound after a straight run of this length, in steps of 0.1 m.
double straight_bound(double metres, const OdometryNoise& n = OdometryNoise()) {
  DeadReckoning d(n);
  d.reset_to(0, 0, 0, 0.0, 0.0);
  for (int i = 0; i < static_cast<int>(metres / 0.1 + 0.5); ++i) d.step(0.1, 0.0);
  return d.radius95();
}

// The distance at which the bound first passes a limit.
double distance_to(double limit) {
  DeadReckoning d((OdometryNoise()));
  d.reset_to(0, 0, 0, 0.0, 0.0);
  for (int i = 1; i <= 100000; ++i) {
    d.step(0.1, 0.0);
    if (d.radius95() > limit) return 0.1 * i;
  }
  return -1.0;
}

}  // namespace

RC_TEST("with no noise there is no bound, and with the noise of the distance alone the bound grows as the square root of the distance") {
  OdometryNoise none;
  none.scale_error = 0.0;
  none.turn_error = 0.0;
  none.heading_walk = 0.0;
  RC_CHECK(straight_bound(100.0, none) < 1e-6);
  OdometryNoise scale;
  scale.turn_error = 0.0;
  scale.heading_walk = 0.0;
  const double a = straight_bound(10.0, scale), b = straight_bound(100.0, scale);
  // Independent errors of 1 percent in each step of 0.1 m add like a random walk: ten times the distance, 3.16 times the error.
  RC_CHECK_NEAR(b / a, 3.1623, 0.15);
  RC_CHECK(b > 0.05 && b < 0.12);
}

RC_TEST("with a heading that wanders the bound grows faster than the distance, as its three halves power") {
  const double r10 = straight_bound(10.0);
  const double r100 = straight_bound(100.0);
  rec("r10", r10, 2);
  rec("r100", r100, 2);
  rec("ratio", r100 / r10, 1);
  std::cout << "  95 percent radius: " << r10 << " m after 10 m, " << r100 << " m after 100 m, " << r100 / r10 << " times for ten times the distance\n";
  RC_CHECK(r100 / r10 > 15.0);
  RC_CHECK(r100 / r10 < 40.0);
}

RC_TEST("a Monte Carlo of noisy runs agrees with the bound") {
  const OdometryNoise n;
  const int runs = 400;
  rec("runs", runs, 0);
  rc::core::Rng rng(21);
  std::vector<double> miss;
  for (int r = 0; r < runs; ++r) {
    double x = 0.0, y = 0.0, th = 0.0;
    for (int i = 0; i < 1000; ++i) {
      const double ds = 0.1 * (1.0 + n.scale_error * rng.gaussian());
      th += n.heading_walk * std::sqrt(0.1) * rng.gaussian();
      x += ds * std::cos(th);
      y += ds * std::sin(th);
    }
    miss.push_back(std::hypot(x - 100.0, y));
  }
  std::sort(miss.begin(), miss.end());
  const double mc95 = miss[static_cast<std::size_t>(0.95 * runs)];
  const double bound = straight_bound(100.0);
  rec("mc95", mc95, 2);
  std::cout << "  95 percent of the robots within " << mc95 << " m; the bound says " << bound << " m\n";
  RC_CHECK(mc95 < 1.3 * bound);
  RC_CHECK(mc95 > 0.6 * bound);
}

RC_TEST("turning adds to the bound, and a fix takes it back and it grows again") {
  DeadReckoning straight((OdometryNoise()));
  DeadReckoning turny((OdometryNoise()));
  straight.reset_to(0, 0, 0, 0, 0);
  turny.reset_to(0, 0, 0, 0, 0);
  for (int i = 0; i < 500; ++i) {
    straight.step(0.1, 0.0);
    turny.step(0.1, i % 2 == 0 ? 0.3 : -0.3);
  }
  RC_CHECK(turny.radius95() > straight.radius95());
  DeadReckoning d((OdometryNoise()));
  d.reset_to(0, 0, 0, 0, 0);
  for (int i = 0; i < 1000; ++i) d.step(0.1, 0.0);
  const double before = d.radius95();
  d.reset_to(d.x(), d.y(), d.theta(), 0.05, 0.02);
  rec("after_fix", d.radius95(), 3);
  RC_CHECK(d.radius95() < 0.05 * before);
  for (int i = 0; i < 1000; ++i) d.step(0.1, 0.0);
  RC_CHECK(d.radius95() > 0.5 * before);
}

RC_TEST("the distance a robot may go before it might be 1 m out, or 5 m, is worked out before it sets off") {
  const double d1 = distance_to(1.0);
  const double d5 = distance_to(5.0);
  rec("d_1m", d1, 0);
  rec("d_5m", d5, 0);
  RC_CHECK(d1 > 0.0 && d5 > d1);
  RC_CHECK(d5 / d1 < 5.0);  // not in proportion: the bound grows faster than the distance
  DeadReckoning d((OdometryNoise()));
  d.reset_to(0, 0, 0, 0, 0);
  for (int i = 0; i < static_cast<int>(d1 / 0.1) + 5; ++i) d.step(0.1, 0.0);
  RC_CHECK(d.needs_fix(1.0));
  RC_CHECK(!d.needs_fix(50.0));
}
