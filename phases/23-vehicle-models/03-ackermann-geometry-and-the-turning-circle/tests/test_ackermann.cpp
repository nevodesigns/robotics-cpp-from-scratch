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


namespace {

const double kPi = 3.14159265358979323846;

}  // namespace

RC_TEST("the smallest circle is the wheelbase over the tangent of the largest angle") {
  CarGeometry g;
  const double r = min_turning_radius(g);
  rec("r_min", r, 3);
  RC_CHECK_NEAR(r, 0.3302 / std::tan(0.4189), 1e-9);
  RC_CHECK(r > 0.7 && r < 0.8);
  RC_CHECK_NEAR(turning_radius(0.3302, 0.2), 0.3302 / std::tan(0.2), 1e-12);
  RC_CHECK(turning_radius(0.3302, 0.1) > turning_radius(0.3302, 0.2));
}

RC_TEST("driven at the largest angle the car comes back to where it started after one circumference") {
  CarGeometry g;
  BicycleCar car(g);
  car.reset(1.0, 2.0, 0.5);
  const double r = min_turning_radius(g);
  const double v = 1.0;
  const double t = 2.0 * kPi * r / v;
  const int n = static_cast<int>(t / 0.0005 + 0.5);
  for (int i = 0; i < n; ++i) car.step(v, g.max_steer, t / n);
  RC_CHECK_NEAR(car.x(), 1.0, 2e-3);
  RC_CHECK_NEAR(car.y(), 2.0, 2e-3);
  const double turned = std::remainder(car.theta() - 0.5, 2.0 * kPi);
  RC_CHECK_NEAR(turned, 0.0, 1e-3);
  // A steering angle beyond the limit is held at it.
  BicycleCar a(g);
  BicycleCar b(g);
  a.step(1.0, 1.2, 0.1);
  b.step(1.0, g.max_steer, 0.1);
  RC_CHECK_NEAR(a.theta(), b.theta(), 1e-15);
}

RC_TEST("the two front wheels are at different angles, and the difference is what the Ackermann condition says") {
  CarGeometry g;
  const double r = min_turning_radius(g);
  double inner = 0.0, outer = 0.0;
  ackermann_angles(g, r, inner, outer);
  rec("inner", inner * 180.0 / kPi, 1);
  rec("outer", outer * 180.0 / kPi, 1);
  rec("diff", (inner - outer) * 180.0 / kPi, 1);
  RC_CHECK(inner > g.max_steer && outer < g.max_steer);
  // cot(outer) - cot(inner) = B / L: the condition that all four wheels roll about one point.
  RC_CHECK_NEAR(1.0 / std::tan(outer) - 1.0 / std::tan(inner), g.track / g.wheelbase, 1e-12);
  // A big circle has two angles that hardly differ.
  double i2 = 0.0, o2 = 0.0;
  ackermann_angles(g, 20.0, i2, o2);
  RC_CHECK((i2 - o2) * 180.0 / kPi < 0.1);
  RC_CHECK_NEAR(o2, i2, 1e-3);
}

RC_TEST("the lane a car needs at its smallest circle is wider than the car") {
  CarGeometry g;
  const double r = min_turning_radius(g);
  const double outer = swept_radius_outer(g, r);
  const double inner = r - 0.5 * g.track;
  rec("swept", outer, 3);
  rec("lane", outer - inner - g.track, 3);
  RC_CHECK_NEAR(outer, std::sqrt((r + 0.1) * (r + 0.1) + 0.3302 * 0.3302), 1e-12);
  RC_CHECK(outer - inner > g.track);
  RC_CHECK_NEAR(outer - inner - g.track, outer - r - 0.5 * g.track, 1e-12);
}

RC_TEST("a curvature becomes a steering angle within the limit, and the surface limits the speed round it") {
  CarGeometry g;
  RC_CHECK_NEAR(curvature_to_steer(g, 0.5), std::atan(0.3302 * 0.5), 1e-12);
  RC_CHECK_NEAR(curvature_to_steer(g, 10.0), g.max_steer, 1e-12);
  RC_CHECK_NEAR(curvature_to_steer(g, -10.0), -g.max_steer, 1e-12);
  const double r = min_turning_radius(g);
  const double vmax = max_speed_in_turn(0.45, r);
  rec("vmax", vmax, 2);
  RC_CHECK_NEAR(vmax, std::sqrt(0.45 * 9.81 * r), 1e-12);
  RC_CHECK(max_speed_in_turn(0.1, r) < max_speed_in_turn(0.8, r));
}
