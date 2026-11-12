#include <rc/test/rc_test.hpp>

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <limits>

#include "solution.hpp"

namespace {

const double kNaN = std::numeric_limits<double>::quiet_NaN();
const double kInf = std::numeric_limits<double>::infinity();

Reading rd(double range, double age = 0.0) {
  Reading r;
  r.range = range;
  r.age = age;
  return r;
}

}  // namespace

RC_TEST("a reading that is not evidence is a blocked path, and a reading of nothing is not an empty one") {
  const LimiterConfig c;
  struct Case { const char* what; Reading r; };
  const Case cases[] = {{"2.0 m", rd(2.0)}, {"not a number", rd(kNaN)}, {"0.0 m", rd(0.0)}, {"0.05 m, inside the blind zone", rd(0.05)}, {"-1.0 m", rd(-1.0)},
                        {"no return (infinity)", rd(kInf)}, {"50 m, beyond the sensor's reach", rd(50.0)}, {"2.0 m but 0.5 s old", rd(2.0, 0.5)}, {"age not a number", rd(2.0, kNaN)}};
  const double want[] = {2.0, 0.0, 0.0, 0.0, 0.0, 1.5, 1.5, 0.0, 0.0};
  std::cout << "  reading                            trusted as\n";
  for (int i = 0; i < 9; ++i) {
    const double t = trusted_range(cases[i].r, c);
    std::cout << "  " << cases[i].what << std::string(34 - std::string(cases[i].what).size(), ' ') << std::fixed << std::setprecision(1) << t << " m\n";
    RC_CHECK_NEAR(t, want[i], 1e-12);
  }
  std::cout << "\n";
}

RC_TEST("the obvious way of using a range lets a reading that is not a number through at full speed") {
  const LimiterConfig c;
  const double naive = std::min(c.top_speed, rc::safety::max_speed(kNaN, c.decel, c.latency, c.margin));
  const double ours = limit_speed(2.0, rd(kNaN), rd(5.0), c);
  std::cout << "  command 2 m/s, front reading not a number: min(top speed, max_speed(NaN)) gave " << naive << " m/s; the limiter gave " << ours << "\n\n";
  RC_CHECK(naive == 2.0);          // std::min returns the first of two when a comparison with NaN is false
  RC_CHECK(ours == 0.0);
  const double naive_inf = std::min(c.top_speed, rc::safety::max_speed(kInf, c.decel, c.latency, c.margin));
  RC_CHECK(naive_inf == 2.0);
  RC_CHECK(limit_speed(2.0, rd(kInf), rd(5.0), c) < 1.5);      // nothing returned: not the whole sensor reach
}

RC_TEST("the speed is limited by the range in the direction of travel, with the sign kept, and a command of zero stays zero") {
  const LimiterConfig c;
  const Reading front = rd(5.0);
  const Reading rear = rd(0.5);
  const double fwd = limit_speed(2.0, front, rear, c);
  const double rev = limit_speed(-2.0, front, rear, c);
  std::cout << "  front clear 5.0 m, rear 0.5 m: forward 2 m/s allowed " << fwd << ", reverse 2 m/s allowed " << rev << "\n";
  RC_CHECK(fwd > 0.0);
  RC_CHECK(rev <= 0.0);
  RC_CHECK(fwd == 2.0 || fwd < 2.0);
  RC_CHECK(std::fabs(rev) < std::fabs(fwd));
  RC_CHECK(limit_speed(0.0, front, rear, c) == 0.0);
  RC_CHECK(limit_speed(kNaN, front, rear, c) == 0.0);
  RC_CHECK(limit_speed(-0.5, rd(0.5), rd(10.0), c) == -0.5);   // a slow command that is allowed passes unchanged
  RC_CHECK(limit_speed(0.5, rd(0.3), rd(10.0), c) == 0.0);      // at the margin: stop
  std::cout << "  a command below what is allowed passes unchanged; at the margin the limit is " << limit_speed(0.5, rd(0.3), rd(10.0), c) << "\n\n";
}

RC_TEST("driving at a wall with the limiter on, the robot slows to a stop short of it, and without it, it arrives at full speed") {
  const LimiterConfig c;
  const double dt = 0.01;
  double min_range = 1e9;
  double impact_speed = 0.0;
  // Limited: the robot follows the limit but cannot brake harder than its deceleration.
  {
    double x = 0.0;
    double v = 0.0;
    const double wall = 10.0;
    for (int i = 0; i < 4000; ++i) {
      const double range = wall - x;
      const double lim = limit_speed(2.0, rd(range), rd(5.0), c);
      v = lim < v ? std::max(lim, v - c.decel * dt) : lim;
      x += v * dt;
      min_range = std::min(min_range, wall - x);
    }
  }
  // Not limited.
  {
    double x = 0.0;
    const double v = 2.0;
    const double wall = 10.0;
    for (int i = 0; i < 4000 && x < wall; ++i) x += v * dt;
    impact_speed = x >= wall ? v : 0.0;
  }
  std::cout << "  command 2 m/s towards a wall 10 m away, margin 0.3 m: with the limiter the closest it came was " << std::fixed << std::setprecision(2) << min_range << " m; without it, it hit at " << impact_speed << " m/s\n\n";
  RC_CHECK(min_range >= c.margin - 0.02);
  RC_CHECK(min_range < c.margin + 0.5);
  RC_CHECK(impact_speed == 2.0);
}

RC_TEST("a sensor that goes quiet in the middle of the approach stops the robot, and one whose readings go stale does too") {
  const LimiterConfig c;
  // A reading that was fine, then not a number, then stale.
  const double ok = limit_speed(2.0, rd(4.0, 0.05), rd(5.0), c);
  const double nan_now = limit_speed(2.0, rd(kNaN, 0.05), rd(5.0), c);
  const double stale = limit_speed(2.0, rd(4.0, 0.25), rd(5.0), c);
  const double fresh = limit_speed(2.0, rd(4.0, 0.2), rd(5.0), c);
  std::cout << "  a good reading of 4.0 m allows " << ok << " m/s; the same reading 0.25 s old allows " << stale << "; one that is not a number allows " << nan_now << "\n\n";
  RC_CHECK(ok > 1.0);
  RC_CHECK(nan_now == 0.0);
  RC_CHECK(stale == 0.0);
  RC_CHECK(fresh > 1.0);       // exactly at the maximum age is still fresh
}
