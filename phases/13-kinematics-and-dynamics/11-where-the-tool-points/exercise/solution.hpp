#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cmath>
#include <rc/kin/planar_arm.hpp>
#include <rc/math/angles.hpp>

// A two-link arm in the plane can put its tool at a point, and that is all it can say about it: the tool points wherever the angles leave it. Add a third link and the tool has a
// heading as well, and a target is a pose: a place and a direction. The arm now has three angles for three numbers, and it can answer for all of them.
//
// What it gains is a new way to fail. A point can be within reach of the whole arm and the pose still be out of it, because the last link has to point where it is told, and the
// wrist, which is what the first two links must reach, is then somewhere else. The test for "can the arm do this" is the test of the WRIST, not of the tool.

struct Pose2 {
  double x = 0.0;
  double y = 0.0;
  double phi = 0.0;     // the heading of the tool, in radians from the x axis
};

struct Arm3Solution {
  double q1 = 0.0;
  double q2 = 0.0;
  double q3 = 0.0;
};

struct Arm3Solutions {
  Arm3Solution elbow_up;
  Arm3Solution elbow_down;
};

// Where the tool is and which way it points, for the three link lengths and the three angles. Each angle is measured from the link before it, so the heading is their sum.
inline Pose2 forward3(double l1, double l2, double l3, double q1, double q2, double q3) {
  // TODO: x = l1 cos(q1) + l2 cos(q1 + q2) + l3 cos(q1 + q2 + q3), and y the same with sin. The heading phi is q1 + q2 + q3: each angle is measured from the link before it.
  (void)l1;
  (void)l2;
  (void)l3;
  (void)q1;
  (void)q2;
  (void)q3;
  return Pose2{-99.0, -99.0, -99.0};
}

// How far the heading `actual` is from the heading `target`, the short way round: in (-pi, pi]. A heading of 3.14 and one of -3.14 are almost the same direction, and their
// difference as numbers is 6.28.
inline double heading_error(double target, double actual) {
  // TODO: rc::math::wrap_angle(target - actual).
  (void)target;
  (void)actual;
  return 99.0;
}

// The two ways to reach a pose, or why it cannot be. The wrist (the end of the second link) has to be where the tool is, less the last link laid along the heading. The first two
// links reach the wrist as a two link arm does, and the third angle is whatever turns the heading into the one that was asked for, wrapped.
inline rc::expected<Arm3Solutions, rc::kin::ReachError> solve3(const Pose2& target, double l1, double l2, double l3) {
  // TODO: the wrist is the tool less the last link along the heading: wx = target.x - l3 cos(phi), wy = target.y - l3 sin(phi). wrist = rc::kin::solve(wx, wy, l1, l2): if it is an
  // error, return rc::unexpected(wrist.error()). Otherwise for each of wrist.value().elbow_up and elbow_down: q1 and q2 as they are, and q3 = rc::math::wrap_angle(phi - q1 - q2).
  (void)target;
  (void)l1;
  (void)l2;
  (void)l3;
  return rc::unexpected(rc::kin::ReachError::TooFar);
}

#endif  // LESSON_SOLUTION_HPP
