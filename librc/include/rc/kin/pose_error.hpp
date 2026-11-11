// rc/kin/pose_error.hpp
//
// How wrong a pose is, from lesson 13-13, graduated.
//
// A yaw of 359 degrees against -1 was a difference of 360 as Euler angles and a rotation vector of length under 1e-9; rotations of 1e-10 were read back by the matrix of the antisymmetric part and lost by the arc cosine; a half turn about x was pi along x; a pose that was right in position and a tenth of a radian out in orientation was not converged.

#ifndef RC_KIN_POSE_ERROR_HPP
#define RC_KIN_POSE_ERROR_HPP

#include <algorithm>
#include <cmath>
#include <rc/kin/dh.hpp>
#include <rc/math/rotation.hpp>
#include <rc/math/vector.hpp>

namespace rc {
namespace kin {

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
  const double (*m)[3] = r.m;
  const rc::math::Vec3 vee{m[2][1] - m[1][2], m[0][2] - m[2][0], m[1][0] - m[0][1]};   // 2 sin(angle) times the axis
  const double trace = m[0][0] + m[1][1] + m[2][2];
  double cos_angle = (trace - 1.0) / 2.0;
  cos_angle = std::max(-1.0, std::min(1.0, cos_angle));
  const double angle = std::acos(cos_angle);
  const double kPi = 3.14159265358979323846;

  if (angle < 1e-7) {
    // Nearly no turn. acos of a number that rounds to 1 is 0, and the information is in the antisymmetric part: for a small angle it is 2 sin(angle), which is 2 angle.
    return rc::math::Vec3{0.5 * vee.x, 0.5 * vee.y, 0.5 * vee.z};
  }
  if (angle > kPi - 1e-4) {
    // Nearly a half turn. sin(angle) is nearly 0, and dividing the antisymmetric part by it divides noise by noise. The symmetric part holds the axis: R = 2 k kT - I at a half turn.
    int k = 0;
    if (m[1][1] > m[k][k]) k = 1;
    if (m[2][2] > m[k][k]) k = 2;
    double axis[3];
    axis[k] = std::sqrt(std::max(0.0, (m[k][k] - cos_angle) / (1.0 - cos_angle)));
    for (int i = 0; i < 3; ++i)
      if (i != k) axis[i] = (m[i][k] + m[k][i]) / (2.0 * (1.0 - cos_angle) * axis[k]);
    const double along = axis[0] * vee.x + axis[1] * vee.y + axis[2] * vee.z;
    const double sign = along < 0.0 ? -1.0 : 1.0;                   // the antisymmetric part, small as it is, still says which way
    return rc::math::Vec3{sign * angle * axis[0], sign * angle * axis[1], sign * angle * axis[2]};
  }
  const double scale = angle / (2.0 * std::sin(angle));
  return rc::math::Vec3{scale * vee.x, scale * vee.y, scale * vee.z};
}

struct PoseError {
  rc::math::Vec3 position;     // target minus current, in the base frame
  rc::math::Vec3 rotation;     // the turn from the current orientation to the target, as a rotation vector in the base frame

  double position_norm() const { return std::sqrt(position.x * position.x + position.y * position.y + position.z * position.z); }
  double rotation_angle() const { return std::sqrt(rotation.x * rotation.x + rotation.y * rotation.y + rotation.z * rotation.z); }
};

// How far `current` is from `target`: the difference of the positions, and the rotation vector of target times the transpose of current.
inline PoseError pose_error(const Frame& current, const Frame& target) {
  PoseError e;
  e.position = rc::math::Vec3{target.p.x - current.p.x, target.p.y - current.p.y, target.p.z - current.p.z};
  e.rotation = rotation_vector(rc::math::multiply(target.r, rc::math::transposed(current.r)));
  return e;
}

// Close enough in both: within `position_tolerance` metres and within `rotation_tolerance` radians. Either alone is not close enough.
inline bool converged(const PoseError& e, double position_tolerance, double rotation_tolerance) {
  return e.position_norm() <= position_tolerance && e.rotation_angle() <= rotation_tolerance;
}
}  // namespace kin
}  // namespace rc

#endif  // RC_KIN_POSE_ERROR_HPP
