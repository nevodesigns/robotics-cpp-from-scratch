// rc/nav/calibrate.hpp
//
// The odometry errors, from lesson 16-09, graduated.
//
// Odometry believes three things it has not measured: that each wheel travels what its
// encoder says, and that the wheels are as far apart as the drawing says. Each is a percent
// or two out on a real robot, and they add. Measured on a square four metres on a side, 16 m
// in all, a robot with a left wheel 1.2 percent big, a right wheel 0.6 percent small and a wheel
// base 2 percent wide ended 249.9 cm from where its odometry thought it was, and 46.4 degrees
// out in heading.
//
// Two moves find the three numbers: a straight drive, which gives the average wheel size and the
// difference between the wheels, and a spin, which gives the wheelbase and is the only move that
// can. They are found in that order, since each needs the one before. With a tape good to 3 mm and
// a protractor good to 0.2 degrees each came out within 0.16 percent of the truth, and the same
// square closed to 2.3 cm and 0.4 degrees.

#ifndef RC_NAV_CALIBRATE_HPP
#define RC_NAV_CALIBRATE_HPP

#include <cmath>

namespace rc {
namespace nav {

// How wrong the robot's idea of itself is, as three numbers that multiply what it believes.
//
// Odometry believes three things it has not measured: that each wheel travels the distance
// its encoder says, and that the wheels are the distance apart the drawing says. Each is a
// little wrong on a real robot: tyres wear and inflate unevenly, the contact patch is not
// where the drawing puts it. A wheel that is one percent bigger than assumed travels one
// percent further than the encoder reports, every revolution, for ever.
struct WheelErrors {
  double left_scale = 1.0;         // what the left wheel really travels per metre its encoder reports
  double right_scale = 1.0;        // the same for the right
  double wheelbase_scale = 1.0;    // the real spacing of the wheels over the believed one
  bool valid = false;              // false when the moves were too small to say anything
};

// A straight drive: both wheels told to travel `nominal_distance` by their encoders. The
// ground distance actually covered, and how far the robot actually turned (counter clockwise
// positive), were measured from outside, with a tape and a protractor, or a camera.
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
//
// Driving straight tells two things. How far it went says how big the wheels are on average.
// How much it veered says how different they are: the wheel that travels further turns the
// robot away from itself. Spinning tells a third: the rotation depends on how far apart the
// wheels are, and is the only move that does.
//
// The three are found in order, because each needs the one before. The average wheel size from
// the straight drive, the wheelbase from the spin and that average, and the difference between
// the wheels from the veer and that wheelbase.
inline WheelErrors calibrate(double wheel_base, const StraightTest& straight, const SpinTest& spin) {
  WheelErrors e;
  if (wheel_base <= 0.0 || straight.nominal_distance == 0.0 || spin.measured_rotation == 0.0 ||
      spin.nominal_travel == 0.0) {
    return e;
  }

  // The robot drove an arc, and what was measured is the chord of it. For a small veer the two
  // differ by the sinc of half the angle, which is worth taking out and costs nothing.
  const double half = 0.5 * straight.heading_change;
  const double chord_over_arc = std::fabs(half) < 1e-9 ? 1.0 : std::sin(half) / half;
  const double arc = straight.measured_distance / chord_over_arc;

  // The average of the two wheel scales.
  const double mean_scale = arc / straight.nominal_distance;

  // Spinning with the wheels travelling opposite ways, the rotation is the sum of the two
  // wheels' real travel over the real wheelbase: nominal travel times (left + right scales),
  // which is twice the mean, over the real wheelbase.
  e.wheelbase_scale = 2.0 * mean_scale * spin.nominal_travel / (spin.measured_rotation * wheel_base);

  // The veer in a straight drive is the difference of the two wheels' real travel over the real
  // wheelbase.
  const double real_base = e.wheelbase_scale * wheel_base;
  const double difference = straight.heading_change * real_base / straight.nominal_distance;

  e.left_scale = mean_scale - 0.5 * difference;
  e.right_scale = mean_scale + 0.5 * difference;
  e.valid = true;
  return e;
}

// What the wheels really travelled, from what the encoders reported.
struct WheelTravel {
  double left = 0.0;
  double right = 0.0;
};

inline WheelTravel corrected(const WheelErrors& e, double left, double right) {
  WheelTravel t;
  t.left = left * e.left_scale;
  t.right = right * e.right_scale;
  return t;
}

}  // namespace nav
}  // namespace rc

#endif  // RC_NAV_CALIBRATE_HPP
