#include <rc/test/rc_test.hpp>

#include <cmath>
#include <iostream>
#include <vector>

#include "solution.hpp"

RC_TEST("a move that is long enough is a ramp, a steady speed and a ramp, and takes the time that adds up") {
  const Trapezoid p = plan_trapezoid(2.0, 1.0, 2.0);
  std::cout << "  distance 2, speed limit 1, acceleration limit 2: ramps of " << p.t_accel << " s, cruise of " << p.t_cruise << " s, total " << p.t_total << " s\n";
  RC_CHECK(p.valid);
  RC_CHECK(!p.triangular());
  RC_CHECK_NEAR(p.v_peak, 1.0, 1e-12);
  RC_CHECK_NEAR(p.t_accel, 0.5, 1e-12);
  RC_CHECK_NEAR(p.t_cruise, 1.5, 1e-12);
  RC_CHECK_NEAR(p.t_total, 2.5, 1e-12);
  RC_CHECK_NEAR(position_at(p, 0.0), 0.0, 1e-12);
  RC_CHECK_NEAR(position_at(p, 0.25), 0.0625, 1e-12);         // 0.5 * 2 * 0.25^2
  RC_CHECK_NEAR(position_at(p, 0.5), 0.25, 1e-12);            // the end of the first ramp
  RC_CHECK_NEAR(position_at(p, 1.0), 0.75, 1e-12);            // 0.25 plus half a second at speed 1
  RC_CHECK_NEAR(position_at(p, 2.0), 1.75, 1e-12);            // a quarter before the end, half a second of the last ramp
  RC_CHECK_NEAR(position_at(p, 2.5), 2.0, 1e-12);
  RC_CHECK_NEAR(position_at(p, 10.0), 2.0, 1e-12);            // after the end it stays
  RC_CHECK_NEAR(position_at(p, -1.0), 0.0, 1e-12);            // and before the start
}

RC_TEST("a move that is too short to reach the speed limit is a triangle with a lower peak, and is quicker than a trapezoid would say") {
  const Trapezoid p = plan_trapezoid(0.25, 1.0, 2.0);
  std::cout << "  distance 0.25: " << (p.triangular() ? "a triangle" : "a trapezoid") << ", peak speed " << p.v_peak << ", total " << p.t_total << " s\n";
  RC_CHECK(p.triangular());
  RC_CHECK_NEAR(p.v_peak, std::sqrt(0.5), 1e-12);
  RC_CHECK_NEAR(p.t_total, std::sqrt(0.5), 1e-9);                       // two ramps of 0.3536
  RC_CHECK_NEAR(position_at(p, p.t_total / 2.0), 0.125, 1e-12);            // half the distance at half the time
  RC_CHECK_NEAR(position_at(p, p.t_total), 0.25, 1e-12);
  // The boundary: a distance exactly twice the ramp distance is a triangle that just reaches the limit.
  const Trapezoid edge = plan_trapezoid(0.5, 1.0, 2.0);
  RC_CHECK(edge.triangular());
  RC_CHECK_NEAR(edge.v_peak, 1.0, 1e-12);
  RC_CHECK_NEAR(edge.t_total, 1.0, 1e-12);
  // A formula for the long case applied to the short one gives a cruise that is less than nothing.
  const double naive_cruise = (0.25 - 2.0 * (1.0 * 1.0 / (2.0 * 2.0))) / 1.0;
  RC_CHECK(naive_cruise < 0.0);
}

RC_TEST("along the move the speed never passes the limit, the acceleration never passes its limit, the position only goes forward, and it ends where it was told") {
  const double vmax = 1.3, amax = 2.7;
  int bad_speed = 0, bad_accel = 0, backwards = 0, ends_wrong = 0, moves = 0;
  const double distances[] = {0.01, 0.2, 0.62, 0.63, 1.0, 3.0, 12.0, -0.4, -5.0};
  for (double d : distances) {
    const Trapezoid p = plan_trapezoid(d, vmax, amax);
    ++moves;
    const double h = 1e-4;
    double previous = 0.0;
    for (double t = 0.0; t <= p.t_total; t += 0.01) {
      const double s0 = position_at(p, t), s1 = position_at(p, t + h), s2 = position_at(p, t + 2.0 * h);
      const double v = (s1 - s0) / h;
      const double a = (s2 - 2.0 * s1 + s0) / (h * h);
      if (std::fabs(v) > vmax + 1e-3) ++bad_speed;
      if (std::fabs(a) > amax + 1e-3 && std::fabs(a) < 1e3) ++bad_accel;        // not counting the corner steps, where the difference spans a change
      if ((d > 0 && s0 < previous - 1e-12) || (d < 0 && s0 > previous + 1e-12)) ++backwards;
      previous = s0;
    }
    if (std::fabs(position_at(p, p.t_total) - d) > 1e-12) ++ends_wrong;
  }
  std::cout << "  " << moves << " moves sampled: " << bad_speed << " over the speed limit, " << bad_accel << " over the acceleration limit, " << backwards << " backwards steps, " << ends_wrong << " ending in the wrong place\n\n";
  RC_CHECK_EQ(moves, 9);
  RC_CHECK_EQ(bad_speed, 0);
  RC_CHECK_EQ(bad_accel, 0);
  RC_CHECK_EQ(backwards, 0);
  RC_CHECK_EQ(ends_wrong, 0);
}

RC_TEST("joints with different distances, each at its own limits, arrive at different times, and given times that make them arrive together they all take the slowest") {
  const std::vector<double> distances = {2.0, 0.5};
  const std::vector<double> vmax = {1.0, 1.0};
  const std::vector<double> amax = {2.0, 2.0};
  const Trapezoid first = plan_trapezoid(2.0, 1.0, 2.0);
  const Trapezoid second = plan_trapezoid(0.5, 1.0, 2.0);
  std::cout << "  alone: " << first.t_total << " s and " << second.t_total << " s\n";
  RC_CHECK_NEAR(first.t_total, 2.5, 1e-12);
  RC_CHECK_NEAR(second.t_total, 1.0, 1e-12);

  const Synchronised s = synchronise(distances, vmax, amax);
  RC_CHECK_NEAR(s.duration, 2.5, 1e-12);
  RC_CHECK_EQ(s.joints.size(), 2u);
  if (s.joints.size() != 2) return;
  std::cout << "  synchronised: both " << s.joints[0].t_total << " s, the second at a peak of " << s.joints[1].v_peak << " instead of " << second.v_peak << "\n\n";
  RC_CHECK_NEAR(s.joints[0].t_total, 2.5, 1e-12);
  RC_CHECK_NEAR(s.joints[1].t_total, 2.5, 1e-12);
  RC_CHECK_NEAR(s.joints[0].v_peak, 1.0, 1e-12);                          // the slowest joint is unchanged
  RC_CHECK_NEAR(s.joints[1].v_peak, (5.0 - std::sqrt(21.0)) / 2.0, 1e-12);    // 0.2087: (aT - sqrt(a^2 T^2 - 4 a d)) / 2
  RC_CHECK_NEAR(position_at(s.joints[0], 2.5), 2.0, 1e-12);
  RC_CHECK_NEAR(position_at(s.joints[1], 2.5), 0.5, 1e-12);               // both are where they were told at the same instant
  RC_CHECK(position_at(s.joints[1], 1.0) < 0.5);                           // and the second has not already got there at 1.0 s
  // The second joint stays inside its own limits.
  RC_CHECK(s.joints[1].v_peak <= 1.0 + 1e-12);
  RC_CHECK_NEAR(s.joints[1].accel, 2.0, 1e-12);
}

RC_TEST("a joint that does not move stays still, a negative distance goes the other way, and limits that are not positive are not a move") {
  const Trapezoid still = plan_trapezoid(0.0, 1.0, 2.0);
  RC_CHECK(still.valid);
  RC_CHECK_NEAR(still.t_total, 0.0, 1e-15);
  RC_CHECK_NEAR(position_at(still, 5.0), 0.0, 1e-15);

  const Trapezoid back = plan_trapezoid(-2.0, 1.0, 2.0);
  RC_CHECK_NEAR(back.t_total, 2.5, 1e-12);
  RC_CHECK_NEAR(position_at(back, 1.0), -0.75, 1e-12);
  RC_CHECK_NEAR(position_at(back, 2.5), -2.0, 1e-12);

  RC_CHECK(!plan_trapezoid(1.0, 0.0, 2.0).valid);
  RC_CHECK(!plan_trapezoid(1.0, 1.0, 0.0).valid);
  RC_CHECK(!plan_trapezoid(1.0, -1.0, 2.0).valid);
  RC_CHECK_NEAR(position_at(plan_trapezoid(1.0, 0.0, 2.0), 1.0), 0.0, 1e-15);

  // Three joints, one of which does not move: the move takes the time of the other two, and that joint stays at 0.
  const Synchronised s = synchronise({1.0, 0.0, -3.0}, {1.0, 1.0, 1.5}, {2.0, 2.0, 3.0});
  RC_CHECK_EQ(s.joints.size(), 3u);
  if (s.joints.size() == 3) {
    RC_CHECK(s.duration > 0.0);
    RC_CHECK_NEAR(position_at(s.joints[1], s.duration), 0.0, 1e-15);
    RC_CHECK_NEAR(position_at(s.joints[2], s.duration), -3.0, 1e-12);
    RC_CHECK_NEAR(position_at(s.joints[0], s.duration), 1.0, 1e-12);
  }
  // A joint with a limit that is not positive: the whole move is not one.
  const Synchronised broken = synchronise({1.0, 1.0}, {1.0, 0.0}, {2.0, 2.0});
  RC_CHECK_NEAR(broken.duration, 0.0, 1e-15);
  RC_CHECK_EQ(broken.joints.size(), 0u);
}
