#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <algorithm>
#include <cmath>
#include <rc/kin/dh.hpp>
#include <rc/math/rotation.hpp>
#include <rc/math/vector.hpp>

// A controller that moves a tool to a pose needs a number for how far it has to go, and for position it is a subtraction. For orientation it is not. Orientations do not add or subtract: the
// difference of two sets of Euler angles is not the angle between the orientations, and a yaw of 359 degrees against one of -1 degree is a difference of 360 and no difference at all.
//
// The way to say how far one orientation is from another is the turn that takes one to the other: the rotation matrix of the target times the transpose of the current one is that turn, and
// the turn can be written as a vector whose direction is the axis and whose length is the angle. That vector is small when the orientations are close, zero when they are the same, and
// does not care how they were described. It has two places where the obvious way to compute it fails: near zero, where an arc cosine throws away the answer, and near a half turn, where the
// axis cannot be read from the part of the matrix that usually holds it.

using rc::kin::Frame;

// The rotation as a vector: the axis of the turn times its angle, with the angle in [0, pi]. The identity is the zero vector.
inline rc::math::Vec3 rotation_vector(const rc::math::Mat3& r) {
  // TODO: vee = (m[2][1] - m[1][2], m[0][2] - m[2][0], m[1][0] - m[0][1]), which is 2 sin(angle) times the axis. cos_angle = (trace - 1) / 2, kept inside [-1, 1]; angle = acos(cos_angle).
  //  - angle < 1e-7: the vector is 0.5 * vee (for a small angle 2 sin(angle) is 2 angle, and the acos has thrown the answer away).
  //  - angle within 1e-4 of pi: sin(angle) is nearly 0. The axis is in the symmetric part: take the largest diagonal element k, axis[k] = sqrt((m[k][k] - cos_angle) / (1 - cos_angle)),
  //    axis[i] = (m[i][k] + m[k][i]) / (2 (1 - cos_angle) axis[k]) for the others. Its sign is that of dot(axis, vee), or plus if that is 0 or positive. The vector is sign * angle * axis.
  //  - otherwise: (angle / (2 sin(angle))) * vee.
  (void)r;
  return rc::math::Vec3{-99.0, -99.0, -99.0};
}

struct PoseError {
  rc::math::Vec3 position;     // target minus current, in the base frame
  rc::math::Vec3 rotation;     // the turn from the current orientation to the target, as a rotation vector in the base frame

  double position_norm() const { return std::sqrt(position.x * position.x + position.y * position.y + position.z * position.z); }
  double rotation_angle() const { return std::sqrt(rotation.x * rotation.x + rotation.y * rotation.y + rotation.z * rotation.z); }
};

// How far `current` is from `target`: the difference of the positions, and the rotation vector of target times the transpose of current.
inline PoseError pose_error(const Frame& current, const Frame& target) {
  // TODO: position = target.p - current.p. rotation = rotation_vector(multiply(target.r, transposed(current.r))): the turn that takes the current orientation to the target.
  (void)current;
  (void)target;
  PoseError e;
  e.position = rc::math::Vec3{-99.0, -99.0, -99.0};
  return e;
}

// Close enough in both: within `position_tolerance` metres and within `rotation_tolerance` radians. Either alone is not close enough.
inline bool converged(const PoseError& e, double position_tolerance, double rotation_tolerance) {
  // TODO: e.position_norm() <= position_tolerance AND e.rotation_angle() <= rotation_tolerance.
  (void)e;
  (void)position_tolerance;
  (void)rotation_tolerance;
  return true;
}

#endif  // LESSON_SOLUTION_HPP
