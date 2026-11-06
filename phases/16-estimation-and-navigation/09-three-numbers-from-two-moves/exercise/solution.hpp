#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cmath>

// How wrong the robot's idea of itself is, as three numbers that multiply what it believes.
struct WheelErrors {
  double left_scale = 1.0;         // what the left wheel really travels per metre its encoder reports
  double right_scale = 1.0;        // the same for the right
  double wheelbase_scale = 1.0;    // the real spacing of the wheels over the believed one
  bool valid = false;              // false when the moves were too small to say anything
};

// A straight drive: both wheels told to travel `nominal_distance` by their encoders. The
// ground distance actually covered, and how far the robot actually turned (counter clockwise
// positive), were measured from outside.
struct StraightTest {
  double nominal_distance = 0.0;
  double measured_distance = 0.0;
  double heading_change = 0.0;
};

// A spin on the spot: the wheels told to travel `nominal_travel` each, in opposite directions.
// How far the robot actually turned was measured from outside.
struct SpinTest {
  double nominal_travel = 0.0;
  double measured_rotation = 0.0;
};

// Three numbers from two moves.
inline WheelErrors calibrate(double wheel_base, const StraightTest& straight, const SpinTest& spin) {
  // TODO: a move of nothing says nothing: if the wheel base, either nominal distance or travel,
  // or the measured rotation is zero (or the base is not positive), return the defaults with
  // valid false, and divide by nothing.
  //
  // Then the three, in order, because each needs the one before:
  //
  // 1. The robot drove an arc, and the chord was measured. chord / arc = sin(half) / half,
  //    where half is half the heading change (1.0 if it is about zero). So
  //    arc = measured_distance / that, and the average scale is arc / nominal_distance.
  //
  // 2. Spinning with the wheels opposite, the rotation is nominal_travel times (left + right
  //    scale) over the real wheelbase, and left + right is twice the average. So
  //    wheelbase_scale = 2 * mean * nominal_travel / (measured_rotation * wheel_base).
  //
  // 3. The veer in the straight drive is (right - left) travel over the real wheelbase, so
  //    right - left = heading_change * real_wheelbase / nominal_distance, with the real
  //    wheelbase = wheelbase_scale * wheel_base. Then left = mean - difference / 2 and
  //    right = mean + difference / 2.
  (void)wheel_base;
  (void)straight;
  (void)spin;
  return WheelErrors{};
}

// What the wheels really travelled, from what the encoders reported.
struct WheelTravel {
  double left = 0.0;
  double right = 0.0;
};

inline WheelTravel corrected(const WheelErrors& e, double left, double right) {
  // TODO: each wheel's reported travel times its own scale.
  (void)e;
  (void)left;
  (void)right;
  return WheelTravel{};
}

#endif  // LESSON_SOLUTION_HPP
