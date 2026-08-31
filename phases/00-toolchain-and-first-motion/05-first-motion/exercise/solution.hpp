// A differential drive robot: two wheels, two speeds, one pose.
//
// Implement step() so the tests pass. The mathematics is spelled out in
// docs/en.md under The Concept.

#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cmath>

// Where the robot is, and which way it faces.
struct Pose {
  double x = 0.0;      // metres
  double y = 0.0;      // metres
  double theta = 0.0;  // radians, anticlockwise from the positive x axis
};

// Brings an angle back into the range from minus pi to pi.
//
// This one is written for you. Read it: std::atan2(sin, cos) rebuilds the angle
// from its own sine and cosine, and those are only defined once per turn, so the
// answer always comes back in range.
inline double wrap_angle(double radians) {
  return std::atan2(std::sin(radians), std::cos(radians));
}

// Advances the robot by one time step.
//
//   start       where the robot is now
//   vl, vr      left and right wheel speeds, metres per second
//   wheel_base  distance between the wheels, metres
//   dt          length of the time step, seconds
//
// Returns the new pose. The returned angle must be wrapped.
inline Pose step(const Pose& start, double vl, double vr, double wheel_base, double dt) {
  // TODO: replace this with the real model.
  // 1. forward speed is the average of the wheel speeds
  // 2. turn rate is their difference divided by the wheel base
  // 3. move forward along theta, then turn, then wrap the angle
  (void)vl;
  (void)vr;
  (void)wheel_base;
  (void)dt;
  return start;
}

#endif  // LESSON_SOLUTION_HPP
