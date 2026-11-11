#include <rc/test/rc_test.hpp>

#include <cmath>
#include <iomanip>
#include <iostream>
#include <vector>

#include "solution.hpp"

namespace {

const double kPi = 3.14159265358979323846;

// A route: straight along x for 10 m, a quarter circle to the left of radius `radius`, straight along y for 10 m.
// Points are `spacing` apart along the way.
std::vector<Vec2> route(double radius, double spacing) {
  std::vector<Vec2> p;
  for (double x = 0.0; x < 10.0 - 1e-9; x += spacing) p.push_back(Vec2{x, 0.0});
  const int arc_points = static_cast<int>(std::ceil(radius * kPi / 2.0 / spacing));
  for (int i = 0; i <= arc_points; ++i) {
    const double a = (kPi / 2.0) * i / arc_points;
    p.push_back(Vec2{10.0 + radius * std::sin(a), radius - radius * std::cos(a)});
  }
  const double top = radius;
  for (double y = top + spacing; y <= top + 10.0 + 1e-9; y += spacing) p.push_back(Vec2{10.0 + radius, y});
  return p;
}

// The sideways acceleration felt at each interior point: speed squared times curvature, worked out here.
double worst_lateral(const std::vector<Vec2>& p, const std::vector<double>& v) {
  double worst = 0.0;
  for (std::size_t i = 1; i + 1 < p.size(); ++i) {
    const double a = v[i] * v[i] * curvature_at(p[i - 1], p[i], p[i + 1]);
    if (a > worst) worst = a;
  }
  return worst;
}

// The largest speed gain and the largest speed loss, as acceleration, along the path.
void worst_longitudinal(const std::vector<Vec2>& p, const std::vector<double>& v, double& accel, double& decel) {
  accel = 0.0;
  decel = 0.0;
  for (std::size_t i = 1; i < p.size(); ++i) {
    const double d = distance_between(p[i - 1], p[i]);
    const double a = (v[i] * v[i] - v[i - 1] * v[i - 1]) / (2.0 * d);
    if (a > accel) accel = a;
    if (-a > decel) decel = -a;
  }
}

}  // namespace

RC_TEST("three points on a circle give one over its radius, and points in a line give none") {
  for (double r : {0.5, 1.0, 2.0, 10.0}) {
    const Vec2 a{r * std::cos(0.2), r * std::sin(0.2)};
    const Vec2 b{r * std::cos(0.3), r * std::sin(0.3)};
    const Vec2 c{r * std::cos(0.45), r * std::sin(0.45)};
    RC_CHECK_NEAR(curvature_at(a, b, c), 1.0 / r, 1e-9);
  }
  RC_CHECK_NEAR(curvature_at(Vec2{0, 0}, Vec2{1, 1}, Vec2{2, 2}), 0.0, 1e-12);
  RC_CHECK_NEAR(curvature_at(Vec2{0, 0}, Vec2{0, 0}, Vec2{2, 2}), 0.0, 1e-12);
  RC_CHECK_NEAR(curvature_at(Vec2{0, 0}, Vec2{1, 0}, Vec2{1, 1}), std::sqrt(2.0), 1e-9);   // a right angle with sides 1 and 1
}

RC_TEST("a speed limit comes from the sideways acceleration: speed squared times curvature, and a tighter corner is slower") {
  std::cout << "  max speed 2 m/s, sideways acceleration limited to 1 m/s^2\n  corner radius (m)   speed limit (m/s)   predicted\n";
  bool close = true;
  double previous = 1e9;
  bool slower = true;
  for (double radius : {0.5, 1.0, 2.0, 4.0, 8.0}) {
    const std::vector<Vec2> p = route(radius, 0.05);
    const std::vector<double> limits = speed_limits(p, 2.0, 1.0);
    RC_CHECK(limits.size() == p.size());
    if (limits.size() != p.size()) return;   // not RC_REQUIRE: it would skip the destructors of p and limits
    double lowest = 1e9;
    for (double l : limits) lowest = std::min(lowest, l);
    const double predicted = std::min(2.0, std::sqrt(1.0 * radius));
    std::cout << "  " << std::setw(17) << std::fixed << std::setprecision(1) << radius << "   " << std::setw(17) << std::setprecision(3)
              << lowest << "   " << std::setw(9) << predicted << "\n";
    if (std::fabs(lowest - predicted) > 0.02) close = false;
    if (radius > 0.5 && !(lowest >= previous)) slower = false;
    previous = lowest;
  }
  std::cout << "\n";
  RC_CHECK(close);
  RC_CHECK(slower);
  const std::vector<Vec2> straight = {Vec2{0, 0}, Vec2{1, 0}, Vec2{2, 0}, Vec2{3, 0}};
  const std::vector<double> l = speed_limits(straight, 2.0, 1.0);
  RC_CHECK(l.size() == 4);
  if (l.size() != 4) return;
  RC_CHECK_NEAR(l[1], 2.0, 1e-12);
  RC_CHECK_NEAR(l[0], 2.0, 1e-12);
}

RC_TEST("at a steady top speed the corner throws the rover sideways far past what the wheels grip, and the limits bring it back") {
  const std::vector<Vec2> p = route(1.0, 0.05);
  const std::vector<double> fast(p.size(), 2.0);
  const std::vector<double> limits = speed_limits(p, 2.0, 1.0);
  RC_CHECK(limits.size() == p.size());
  if (limits.size() != p.size()) return;
  const double flat_out = worst_lateral(p, fast);
  const double limited = worst_lateral(p, limits);
  std::cout << "  radius 1 m corner: sideways acceleration at a steady 2 m/s " << std::fixed << std::setprecision(2) << flat_out
            << " m/s^2, with speed limits " << limited << " m/s^2 (the wheels grip 1.00)\n\n";
  RC_CHECK_NEAR(flat_out, 4.0, 0.05);
  RC_CHECK(limited <= 1.0 + 1e-6);
  RC_CHECK(limited > 0.9);
}

RC_TEST("limits alone ask for a stop in a single step, and a profile spreads the braking out within what the wheels can do") {
  const std::vector<Vec2> p = route(1.0, 0.05);
  const std::vector<double> limits = speed_limits(p, 2.0, 1.0);
  RC_CHECK(limits.size() == p.size());
  if (limits.size() != p.size()) return;
  double accel_limits = 0.0;
  double decel_limits = 0.0;
  worst_longitudinal(p, limits, accel_limits, decel_limits);

  const double max_accel = 1.0;
  const double max_decel = 1.5;
  const std::vector<double> profile = speed_profile(p, limits, max_accel, max_decel, 0.0);
  RC_CHECK(profile.size() == p.size());
  if (profile.size() != p.size()) return;
  double accel_profile = 0.0;
  double decel_profile = 0.0;
  worst_longitudinal(p, profile, accel_profile, decel_profile);
  bool under_limits = true;
  for (std::size_t i = 0; i < p.size(); ++i)
    if (profile[i] > limits[i] + 1e-9) under_limits = false;

  std::cout << "  radius 1 m corner, wheels can gain " << max_accel << " and lose " << max_decel << " m/s^2\n"
            << "                  worst gain   worst loss\n"
            << "  limits alone    " << std::fixed << std::setprecision(2) << std::setw(10) << accel_limits << "   " << std::setw(10)
            << decel_limits << "\n  profile         " << std::setw(10) << accel_profile << "   " << std::setw(10) << decel_profile
            << "\n  the profile starts at " << profile.front() << " and ends at " << profile.back() << " m/s\n\n";
  RC_CHECK(under_limits);
  RC_CHECK(decel_limits > 2.0 * max_decel);
  RC_CHECK(accel_profile <= max_accel + 1e-6);
  RC_CHECK(decel_profile <= max_decel + 1e-6);
  RC_CHECK_NEAR(profile.back(), 0.0, 1e-12);
  RC_CHECK_NEAR(profile.front(), 0.0, 1e-12);
  RC_CHECK(speed_profile(std::vector<Vec2>(), std::vector<double>(), 1.0, 1.0, 0.0).empty());
}

RC_TEST("the cost of grip is time: a rover that slows for the corner arrives later, by an amount that is now a number") {
  std::cout << "  radius 1 m corner, 20 m of straight either side: time to drive it\n"
            << "  speed limit on the corner   time (s)\n";
  const std::vector<Vec2> p = route(1.0, 0.05);
  double t_loose = 0.0;
  double t_tight = 0.0;
  for (double lateral : {4.0, 2.0, 1.0, 0.5}) {
    const std::vector<double> limits = speed_limits(p, 2.0, lateral);
    const std::vector<double> v = speed_profile(p, limits, 1.0, 1.5, 0.0);
    const double t = travel_time(p, v);
    std::cout << "  " << std::setw(15) << std::fixed << std::setprecision(2) << std::sqrt(lateral) << " m/s            " << std::setw(7) << t << "\n";
    if (lateral == 4.0) t_loose = t;
    if (lateral == 0.5) t_tight = t;
  }
  std::cout << "\n";
  RC_CHECK(t_loose > 0.0);
  RC_CHECK(t_tight > t_loose);
  RC_CHECK_NEAR(travel_time(p, std::vector<double>(p.size(), 0.0)), -1.0, 1e-12);
  RC_CHECK_NEAR(travel_time(std::vector<Vec2>{Vec2{0, 0}}, std::vector<double>{1.0}), 0.0, 1e-12);
}
