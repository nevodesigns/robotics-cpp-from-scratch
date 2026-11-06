#include <rc/test/rc_test.hpp>

#include <cmath>
#include <iomanip>
#include <iostream>

#include "solution.hpp"

namespace {

const double kPi = 3.14159265358979323846;

// Forward speed 1 m/s and a turn of 1 rad/s: a circle of radius 1 m. A quarter of it takes pi/2 seconds and
// ends at (1, 1) facing up.
const double kWheelBase = 0.5;
const double kLeft = 0.75;
const double kRight = 1.25;
const double kDuration = kPi / 2.0;

double miss(int method, int steps) {
  const Pose end = drive_with(method, Pose{}, kLeft, kRight, kWheelBase, kDuration / steps, steps);
  return std::hypot(end.x - 1.0, end.y - 1.0);
}

double radius_from_centre(int method, int steps) {
  const Pose end = drive_with(method, Pose{}, kLeft, kRight, kWheelBase, kDuration / steps, steps);
  return std::hypot(end.x - 0.0, end.y - 1.0);   // the centre of the circle is (0, 1)
}

}  // namespace

RC_TEST("the step of lesson 00-05 is the first of the three") {
  const Pose a = step_euler(Pose{0.2, 0.3, 0.4}, 0.75, 1.25, 0.5, 0.1);
  const Pose b = rc::sim::step(Pose{0.2, 0.3, 0.4}, 0.75, 1.25, 0.5, 0.1);
  RC_CHECK_NEAR(a.x, b.x, 1e-15);
  RC_CHECK_NEAR(a.y, b.y, 1e-15);
  RC_CHECK_NEAR(a.theta, b.theta, 1e-15);

  // The other two are different steps, and neither is that one.
  const Pose m = step_midpoint(Pose{0.2, 0.3, 0.4}, 0.75, 1.25, 0.5, 0.1);
  RC_CHECK_NEAR(m.x, 0.2 + 0.1 * std::cos(0.45), 1e-12);   // the heading halfway: 0.4 + 0.1 / 2
  RC_CHECK(std::fabs(m.x - a.x) > 1e-5);
}

RC_TEST("one step of the exact arc lands on the circle, however long the step") {
  const Pose end = step_arc(Pose{}, kLeft, kRight, kWheelBase, kDuration);
  RC_CHECK_NEAR(end.x, 1.0, 1e-12);
  RC_CHECK_NEAR(end.y, 1.0, 1e-12);
  RC_CHECK_NEAR(end.theta, kPi / 2.0, 1e-12);
}

RC_TEST("going straight is not a circle of infinite radius") {
  const Pose end = step_arc(Pose{1.0, 2.0, 0.5}, 1.0, 1.0, 0.5, 0.1);
  RC_CHECK_NEAR(end.x, 1.0 + 0.1 * std::cos(0.5), 1e-12);
  RC_CHECK_NEAR(end.y, 2.0 + 0.1 * std::sin(0.5), 1e-12);
  RC_CHECK_NEAR(end.theta, 0.5, 1e-12);
  RC_CHECK(std::isfinite(end.x) && std::isfinite(end.y));
}

RC_TEST("the error of the step of 00-05 halves when the steps are twice as many, and it always runs wide") {
  const int counts[] = {10, 20, 40, 80, 160, 320};
  std::cout << "  a quarter circle of radius 1 m, miss in millimetres\n"
            << "  steps    00-05 step    midpoint     exact arc\n";
  double previous = 0.0;
  bool halves = true;
  bool wide = true;
  for (int n : counts) {
    const double e = miss(kEuler, n);
    std::cout << "  " << std::setw(5) << n << "   " << std::fixed << std::setprecision(3) << std::setw(10) << e * 1000.0
              << "   " << std::setw(10) << miss(kMidpoint, n) * 1000.0 << "   " << std::scientific << std::setprecision(1)
              << std::setw(10) << miss(kArc, n) * 1000.0 << "\n";
    if (previous > 0.0) {
      const double ratio = previous / e;
      if (ratio < 1.9 || ratio > 2.1) halves = false;
    }
    previous = e;
    if (radius_from_centre(kEuler, n) <= 1.0) wide = false;
  }
  std::cout << "\n";
  RC_CHECK(halves);
  RC_CHECK(wide);
  RC_CHECK(miss(kArc, 10) < 1e-9);   // and the comparison is only meaningful if the exact one is exact
}

RC_TEST("the midpoint step's error falls by four when the steps are twice as many") {
  double previous = miss(kMidpoint, 10);
  bool quarters = true;
  const int counts[] = {20, 40, 80, 160};
  for (int n : counts) {
    const double e = miss(kMidpoint, n);
    const double ratio = previous / e;
    if (ratio < 3.8 || ratio > 4.2) quarters = false;
    previous = e;
  }
  RC_CHECK(quarters);
  RC_CHECK(miss(kMidpoint, 10) < miss(kEuler, 10) / 5.0);
}

RC_TEST("steps needed to be within a millimetre: the 00-05 step needs hundreds, the others far fewer") {
  int euler = 0;
  int midpoint = 0;
  int arc = 0;
  for (int n = 1; n <= 3000 && (euler == 0 || midpoint == 0 || arc == 0); ++n) {
    if (euler == 0 && miss(kEuler, n) < 0.001) euler = n;
    if (midpoint == 0 && miss(kMidpoint, n) < 0.001) midpoint = n;
    if (arc == 0 && miss(kArc, n) < 0.001) arc = n;
  }
  std::cout << "  smallest number of steps for a miss under 1 mm: 00-05 step " << euler << ", midpoint " << midpoint
            << ", exact arc " << arc << "\n\n";
  RC_REQUIRE(euler > 0 && midpoint > 0 && arc > 0);
  RC_CHECK(euler > 500);
  RC_CHECK(midpoint < euler / 10);
  RC_CHECK_EQ(arc, 1);
}
