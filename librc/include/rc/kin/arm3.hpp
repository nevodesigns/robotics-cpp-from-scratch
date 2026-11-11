// rc/kin/arm3.hpp
//
// A three link planar arm to a pose, from lesson 13-11, graduated.
//
// With links of 1, 1 and 1 the tool at (2.9, 0) was reachable pointing along the x axis and not pointing back along it, though 2.9 is inside the total reach of 3; at (1.5, 0) 209 of 360 headings were reachable; a heading of 3.14 against -3.14 is 6.28 as a difference and about -0.003 the short way round.

#ifndef RC_KIN_ARM3_HPP
#define RC_KIN_ARM3_HPP

#include <cmath>
#include <rc/kin/planar_arm.hpp>
#include <rc/math/angles.hpp>

namespace rc {
namespace kin {

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
  Pose2 p;
  p.x = l1 * std::cos(q1) + l2 * std::cos(q1 + q2) + l3 * std::cos(q1 + q2 + q3);
  p.y = l1 * std::sin(q1) + l2 * std::sin(q1 + q2) + l3 * std::sin(q1 + q2 + q3);
  p.phi = q1 + q2 + q3;
  return p;
}

// How far the heading `actual` is from the heading `target`, the short way round: in (-pi, pi]. A heading of 3.14 and one of -3.14 are almost the same direction, and their
// difference as numbers is 6.28.
inline double heading_error(double target, double actual) { return rc::math::wrap_angle(target - actual); }

// The two ways to reach a pose, or why it cannot be. The wrist (the end of the second link) has to be where the tool is, less the last link laid along the heading. The first two
// links reach the wrist as a two link arm does, and the third angle is whatever turns the heading into the one that was asked for, wrapped.
inline rc::expected<Arm3Solutions, rc::kin::ReachError> solve3(const Pose2& target, double l1, double l2, double l3) {
  const double wx = target.x - l3 * std::cos(target.phi);
  const double wy = target.y - l3 * std::sin(target.phi);
  const auto wrist = rc::kin::solve(wx, wy, l1, l2);
  if (!wrist) return rc::unexpected(wrist.error());

  Arm3Solutions out;
  const rc::kin::ArmSolution up = wrist.value().elbow_up;
  const rc::kin::ArmSolution down = wrist.value().elbow_down;
  out.elbow_up = Arm3Solution{up.q1, up.q2, rc::math::wrap_angle(target.phi - up.q1 - up.q2)};
  out.elbow_down = Arm3Solution{down.q1, down.q2, rc::math::wrap_angle(target.phi - down.q1 - down.q2)};
  return out;
}
}  // namespace kin
}  // namespace rc

#endif  // RC_KIN_ARM3_HPP
