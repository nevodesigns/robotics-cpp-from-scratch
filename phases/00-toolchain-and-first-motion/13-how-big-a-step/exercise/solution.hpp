#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cmath>

#include <rc/sim/diff_drive.hpp>

using rc::sim::Pose;

// Which way of taking one step. Named numbers and not an enum, which a later lesson introduces.
const int kEuler = 0;      // move along the heading the step started with: what lesson 00-05 wrote
const int kMidpoint = 1;   // move along the heading halfway through the step
const int kArc = 2;        // follow the circle the robot is actually on

// The step of lesson 00-05 again: the heading at the start of the step decides where the whole step goes.
inline Pose step_euler(const Pose& start, double left, double right, double wheel_base, double dt) {
  return rc::sim::step(start, left, right, wheel_base, dt);
}

// Move along the heading the robot has halfway through the step. The robot turns during the step, so the
// heading it had at the start is wrong for the whole of it and so is the heading at the end, and halfway is
// right for both in equal measure.
inline Pose step_midpoint(const Pose& start, double left, double right, double wheel_base, double dt) {
  // TODO: forward = (left + right) / 2 and turn = (right - left) / wheel_base, as always. The new
  // heading is start.theta + turn * dt. But x and y move along the heading HALFWAY through the
  // step, start.theta + turn * dt / 2:
  //   x = start.x + forward * dt * cos(halfway),  y = start.y + forward * dt * sin(halfway).
  (void)left;
  (void)right;
  (void)wheel_base;
  (void)dt;
  return start;
}

// The exact answer for constant wheel speeds: a circle of radius forward / turn.
inline Pose step_arc(const Pose& start, double left, double right, double wheel_base, double dt) {
  // TODO: forward and turn as before. If |turn| is under 1e-9 the robot is going straight: move
  // forward * dt along start.theta and keep the heading. Otherwise radius = forward / turn,
  // end_heading = start.theta + turn * dt, and
  //   x = start.x + radius * (sin(end_heading) - sin(start.theta))
  //   y = start.y - radius * (cos(end_heading) - cos(start.theta))
  // with the heading set to end_heading.
  (void)left;
  (void)right;
  (void)wheel_base;
  (void)dt;
  return start;
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

#endif  // LESSON_SOLUTION_HPP
