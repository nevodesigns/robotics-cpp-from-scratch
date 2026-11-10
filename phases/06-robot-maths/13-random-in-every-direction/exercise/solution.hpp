#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cmath>
#include <vector>

#include <rc/core/random.hpp>
#include <rc/math/quaternion.hpp>
#include <rc/math/vector.hpp>

using rc::math::Quat;
using rc::math::Vec3;

const double kTwoPi = 6.283185307179586;
const double kHalfPi = 1.5707963267948966;

// A direction picked by choosing a latitude and a longitude each evenly. It looks right and is not even: every line
// of latitude is chosen equally often, however short it is, so the poles, where the lines are tiny, are crowded.
inline Vec3 random_direction_naive(rc::core::Rng& rng) {
  const double latitude = (rng.uniform() - 0.5) * 2.0 * kHalfPi;
  const double longitude = rng.uniform() * kTwoPi;
  return Vec3{std::cos(latitude) * std::cos(longitude), std::cos(latitude) * std::sin(longitude), std::sin(latitude)};
}

// A direction picked evenly over the sphere.
//
// A fact worth knowing (Archimedes'): the area of a band of a sphere depends only on its height, not on where it is. So
// choosing the height z evenly between -1 and 1, and the longitude evenly, covers the sphere evenly.
inline Vec3 random_direction(rc::core::Rng& rng) {
  // TODO: z = 2 u - 1 where u is rng.uniform(), longitude = rng.uniform() * kTwoPi (draw them in that order),
  // r = sqrt(max(0, 1 - z^2)). The direction is (r cos(longitude), r sin(longitude), z).
  (void)rng;
  return Vec3{0.0, 0.0, 1.0};
}

// A rotation from three angles each chosen evenly: a roll, a pitch and a yaw. It is not an even choice of rotation, since the
// angle and the direction of the axis it makes are not spread the way rotations are.
inline Quat random_rotation_naive(rc::core::Rng& rng) {
  const double roll = (rng.uniform() - 0.5) * kTwoPi;
  const double pitch = (rng.uniform() - 0.5) * 2.0 * kHalfPi;
  const double yaw = (rng.uniform() - 0.5) * kTwoPi;
  const Quat qx = rc::math::from_axis_angle(Vec3{1, 0, 0}, roll);
  const Quat qy = rc::math::from_axis_angle(Vec3{0, 1, 0}, pitch);
  const Quat qz = rc::math::from_axis_angle(Vec3{0, 0, 1}, yaw);
  return rc::math::multiply(qz, rc::math::multiply(qy, qx));
}

// A rotation chosen evenly (Shoemake's method): three numbers evenly in [0, 1), and two angles and two radii that
// together place a point evenly on the surface of a four dimensional sphere, which is where unit quaternions live.
inline Quat random_rotation(rc::core::Rng& rng) {
  // TODO: Shoemake's method. Draw u1, u2, u3 from rng.uniform() in that order. a = sqrt(1 - u1), b = sqrt(u1).
  // The quaternion is w = b cos(2 pi u3), x = a sin(2 pi u2), y = a cos(2 pi u2), z = b sin(2 pi u3).
  (void)rng;
  return Quat{};
}

// How far the rotation turns, as the angle of the turn about its axis, from 0 to pi.
inline double rotation_angle(const Quat& q) {
  // TODO: 2 * acos(|w|), with |w| held to at most 1 so a tiny rounding error cannot make acos give not a number.
  (void)q;
  return -1.0;
}

// The angle between two directions, in degrees.
inline double angle_between_degrees(const Vec3& a, const Vec3& b) {
  double c = a.x * b.x + a.y * b.y + a.z * b.z;
  if (c > 1.0) c = 1.0;
  if (c < -1.0) c = -1.0;
  return std::acos(c) * 180.0 / 3.14159265358979323846;
}

#endif  // LESSON_SOLUTION_HPP
