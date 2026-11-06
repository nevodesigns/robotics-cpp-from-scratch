// rc/sim/arc.hpp
//
// Three ways to take one step of motion, from lesson 00-13, graduated.
//
// The step of lesson 00-05 moves along the heading the robot had at the start of the step, so on a curve it runs wide.
// Driving a quarter circle of radius 1 m in 10, 20, 40, 80, 160 and 320 steps, it missed by 111.110, 55.541,
// 27.769, 13.884, 6.942 and 3.471 mm: the error halves when the steps double, and it needed 1111 steps to
// be within a millimetre. Moving along the heading halfway through the step missed by 1.455, 0.364, 0.091,
// 0.023, 0.006 and 0.001 mm, which is four times less each time and needed 13 steps. The exact arc, for constant wheel speeds, is
// exact in one step (the miss was under 1e-11 mm at every count), and goes straight when the turn is too small for a radius.

#ifndef RC_SIM_ARC_HPP
#define RC_SIM_ARC_HPP

#include <cmath>

#include <rc/sim/diff_drive.hpp>

namespace rc {
namespace sim {

// Which way of taking one step. Named numbers and not an enum, which a later lesson introduces.
const int kEuler = 0;      // move along the heading the step started with: what lesson 00-05 wrote
const int kMidpoint = 1;   // move along the heading halfway through the step
const int kArc = 2;        // follow the circle the robot is actually on

// The step of lesson 00-05 again: the heading at the start of the step decides where the whole step goes.
inline Pose step_euler(const Pose& start, double left, double right, double wheel_base, double dt) {
  return step(start, left, right, wheel_base, dt);
}

// Move along the heading the robot has halfway through the step. The robot turns during the step, so the
// heading it had at the start is wrong for the whole of it and so is the heading at the end, and halfway is
// right for both in equal measure.
inline Pose step_midpoint(const Pose& start, double left, double right, double wheel_base, double dt) {
  const double forward = (left + right) / 2.0;
  const double turn = (right - left) / wheel_base;

  Pose next;
  const double halfway = start.theta + turn * dt / 2.0;
  next.x = start.x + forward * dt * std::cos(halfway);
  next.y = start.y + forward * dt * std::sin(halfway);
  next.theta = start.theta + turn * dt;
  return next;
}

// The exact answer for constant wheel speeds: a circle of radius forward / turn.
//
// When the turn is too small for the radius to mean anything, the robot is going straight, and the formula
// would divide by nearly zero. Go straight instead.
inline Pose step_arc(const Pose& start, double left, double right, double wheel_base, double dt) {
  const double forward = (left + right) / 2.0;
  const double turn = (right - left) / wheel_base;

  Pose next;
  if (std::fabs(turn) < 1e-9) {
    next.x = start.x + forward * dt * std::cos(start.theta);
    next.y = start.y + forward * dt * std::sin(start.theta);
    next.theta = start.theta;
    return next;
  }

  const double radius = forward / turn;
  const double end_heading = start.theta + turn * dt;
  next.x = start.x + radius * (std::sin(end_heading) - std::sin(start.theta));
  next.y = start.y - radius * (std::cos(end_heading) - std::cos(start.theta));
  next.theta = end_heading;
  return next;
}

// Drive for `steps` steps of `dt` with the chosen method.
inline Pose drive_with(int method, const Pose& start, double left, double right, double wheel_base, double dt,
                       int steps) {
  Pose pose = start;
  for (int i = 0; i < steps; ++i) {
    if (method == kMidpoint) pose = step_midpoint(pose, left, right, wheel_base, dt);
    else if (method == kArc) pose = step_arc(pose, left, right, wheel_base, dt);
    else pose = step_euler(pose, left, right, wheel_base, dt);
  }
  return pose;
}

}  // namespace sim
}  // namespace rc

#endif  // RC_SIM_ARC_HPP
