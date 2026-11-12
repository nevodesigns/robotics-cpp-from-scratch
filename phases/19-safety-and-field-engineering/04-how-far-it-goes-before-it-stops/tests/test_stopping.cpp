#include <rc/test/rc_test.hpp>

#include <cmath>
#include <iomanip>
#include <iostream>

#include "solution.hpp"

namespace {

// The distance travelled by actually integrating a robot that goes on at its speed for the latency and then brakes.
double simulate_stop(double v0, double decel, double latency, double dt) {
  double x = 0.0;
  double v = v0;
  double t = 0.0;
  while (v > 0.0) {
    if (t >= latency) v -= decel * dt;
    if (v < 0.0) v = 0.0;
    x += v * dt;
    t += dt;
  }
  return x;
}

const double kDeg = 3.14159265358979323846 / 180.0;

}  // namespace

RC_TEST("the formula matches a robot that is actually stopped step by step") {
  std::cout << "  deceleration 2 m/s^2, latency 0.3 s\n  speed   formula   simulated   difference\n";
  const double speeds[] = {0.5, 1.0, 2.0, 3.0, 5.0};
  for (double v : speeds) {
    const double f = stopping_distance(v, 2.0, 0.3);
    const double s = simulate_stop(v, 2.0, 0.3, 1e-4);
    std::cout << "  " << std::fixed << std::setprecision(1) << std::setw(4) << v << "   " << std::setprecision(3) << std::setw(7) << f << "   " << std::setw(9) << s << "   " << std::setw(9) << (f - s) << "\n";
    RC_CHECK_NEAR(f, s, 0.005);
  }
  std::cout << "\n";
  RC_CHECK(stopping_distance(0.0, 2.0, 0.3) == 0.0);
  RC_CHECK_NEAR(stopping_distance(-2.0, 2.0, 0.3), stopping_distance(2.0, 2.0, 0.3), 1e-12);
  RC_CHECK(std::isinf(stopping_distance(1.0, 0.0, 0.3)));
  RC_CHECK_NEAR(stopping_distance(2.0, 2.0, -1.0), 1.0, 1e-12);        // a negative latency is no latency
}

RC_TEST("the largest speed for a sight line is the inverse of the stopping distance, with the margin kept") {
  std::cout << "  deceleration 2, latency 0.4 s, margin 0.2 m\n  sight   max speed   stops in\n";
  const double sights[] = {0.2, 0.5, 1.0, 2.0, 5.0, 10.0};
  double previous = -1.0;
  for (double s : sights) {
    const double v = max_speed(s, 2.0, 0.4, 0.2);
    const double d = stopping_distance(v, 2.0, 0.4);
    std::cout << "  " << std::fixed << std::setprecision(1) << std::setw(5) << s << "   " << std::setprecision(3) << std::setw(9) << v << "   " << std::setw(7) << d << "\n";
    RC_CHECK(v >= previous);
    previous = v;
    if (s > 0.2) RC_CHECK_NEAR(d, s - 0.2, 1e-9);
    else RC_CHECK(v == 0.0);
  }
  std::cout << "\n";
  RC_CHECK(max_speed(1.0, 0.0, 0.4, 0.2) == 0.0);
  RC_CHECK(max_speed(0.1, 2.0, 0.4, 0.2) == 0.0);
}

RC_TEST("latency costs speed, and a second of it costs more than two fifths") {
  const double lat[] = {0.0, 0.1, 0.5, 1.0};
  double v0 = 0.0;
  std::cout << "  a sight line of 3 m, deceleration 2, margin 0.2:\n";
  for (double l : lat) {
    const double v = max_speed(3.0, 2.0, l, 0.2);
    if (l == 0.0) v0 = v;
    std::cout << "  latency " << std::fixed << std::setprecision(1) << l << " s: " << std::setprecision(3) << v << " m/s (" << std::setprecision(0) << 100.0 * v / v0 << " percent of the speed with none)\n";
  }
  std::cout << "\n";
  RC_CHECK(max_speed(3.0, 2.0, 1.0, 0.2) < 0.6 * max_speed(3.0, 2.0, 0.0, 0.2));   // 57 percent: a second costs 43
  RC_CHECK(max_speed(3.0, 2.0, 0.1, 0.2) < max_speed(3.0, 2.0, 0.0, 0.2));
}

RC_TEST("the ground and gravity decide the braking, and on a wet slope there is none to be had") {
  std::cout << "  brakes 3 m/s^2: friction and slope -> deceleration, stop from 1.5 m/s (latency 0.3)\n";
  struct Case { const char* what; double mu; double deg; };
  const Case cases[] = {{"dry, flat", 0.8, 0.0}, {"dry, 10 deg downhill", 0.8, 10.0}, {"dry, 10 deg uphill", 0.8, -10.0}, {"wet, flat", 0.3, 0.0}, {"wet, 10 deg downhill", 0.3, 10.0}, {"wet, 17 deg downhill", 0.3, 17.0}};
  double flat = 0.0;
  double wet10 = 0.0;
  for (const Case& c : cases) {
    const double a = effective_decel(3.0, c.mu, c.deg * kDeg);
    const double d = stopping_distance(1.5, a, 0.3);
    std::cout << "  " << c.what << std::string(22 - std::string(c.what).size(), ' ') << std::fixed << std::setprecision(2) << a << " m/s^2  ";
    if (std::isinf(d)) std::cout << "cannot stop\n";
    else std::cout << std::setprecision(2) << d << " m\n";
    if (c.mu == 0.8 && c.deg == 0.0) flat = a;
    if (c.mu == 0.3 && c.deg == 10.0) wet10 = a;
  }
  std::cout << "\n";
  RC_CHECK_NEAR(flat, 3.0, 1e-9);                              // the brakes are the limit on dry ground
  RC_CHECK_NEAR(effective_decel(3.0, 0.3, 0.0), 0.3 * 9.81, 1e-9);   // the ground is the limit when wet
  RC_CHECK(wet10 < 0.5 * flat);
  RC_CHECK(effective_decel(3.0, 0.3, 17.0 * kDeg) == 0.0);
  RC_CHECK(effective_decel(3.0, 0.8, -10.0 * kDeg) > 3.0);     // uphill helps
  RC_CHECK(effective_decel(3.0, 0.8, -10.0 * kDeg) > effective_decel(3.0, 0.8, 0.0));
  RC_CHECK(std::isinf(stopping_distance(1.5, effective_decel(3.0, 0.3, 17.0 * kDeg), 0.3)));
}

RC_TEST("the latency that matters is the whole chain from the event to the brake, and the stop path's timeout is part of it") {
  const double lat = total_latency(0.1, 0.3, 0.01, 0.15);
  RC_CHECK_NEAR(lat, 0.56, 1e-12);
  const double before_brake = 2.0 * lat;
  std::cout << "  sensor 0.1 s, stop path timeout 0.3 s, one tick 0.01 s, brake 0.15 s: " << lat << " s\n";
  std::cout << "  at 2 m/s the robot goes " << before_brake << " m before the brakes act, and " << stopping_distance(2.0, 2.0, lat) << " m in all\n";
  const double v = max_speed(3.0, 2.0, lat, 0.2);
  std::cout << "  the speed a 3 m sight line allows with it: " << v << " m/s (with the timeout cut to 0.1 s: " << max_speed(3.0, 2.0, total_latency(0.1, 0.1, 0.01, 0.15), 0.2) << " m/s)\n\n";
  RC_CHECK_NEAR(stopping_distance(2.0, 2.0, lat), 2.0 * lat + 1.0, 1e-12);
  RC_CHECK(max_speed(3.0, 2.0, total_latency(0.1, 0.1, 0.01, 0.15), 0.2) > v);
}
