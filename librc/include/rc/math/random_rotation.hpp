// rc/math/random_rotation.hpp
//
// Directions and rotations drawn evenly, from lesson 06-13, graduated.
//
// A direction from a latitude and a longitude each chosen evenly crowds the poles: of 100000, 5.46 percent fell within 10 degrees of the north pole where the sphere holds 0.76 percent, 7.2 times too many, and choosing the height evenly gave 0.71 percent. A rotation from a roll,
// a pitch and a yaw each chosen evenly looks right on average and is wrong in its spread: the mean turn was 126.0 degrees against the right 126.5, and the share of turns under 30 degrees was 0.0045 against 0.0075, 40 percent too few. Shoemake's method gave 126.4 and 0.0075, and a pole turned
// by it fell within 10 degrees of itself 0.73 percent of the time (expected 0.76). A check on the average alone would have passed the bad draw.

#ifndef RC_MATH_RANDOM_ROTATION_HPP
#define RC_MATH_RANDOM_ROTATION_HPP

#include <cmath>
#include <vector>

#include <rc/core/random.hpp>
#include <rc/math/quaternion.hpp>
#include <rc/math/vector.hpp>

namespace rc {
namespace math {

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
  const double z = 2.0 * rng.uniform() - 1.0;
  const double longitude = rng.uniform() * kTwoPi;
  const double r = std::sqrt(std::fmax(0.0, 1.0 - z * z));
  return Vec3{r * std::cos(longitude), r * std::sin(longitude), z};
}

// A rotation from three angles each chosen evenly: a roll, a pitch and a yaw. It is not an even choice of rotation, since the
// angle and the direction of the axis it makes are not spread the way rotations are.
inline Quat random_rotation_naive(rc::core::Rng& rng) {
  const double roll = (rng.uniform() - 0.5) * kTwoPi;
  const double pitch = (rng.uniform() - 0.5) * 2.0 * kHalfPi;
  const double yaw = (rng.uniform() - 0.5) * kTwoPi;
  const Quat qx = from_axis_angle(Vec3{1, 0, 0}, roll);
  const Quat qy = from_axis_angle(Vec3{0, 1, 0}, pitch);
  const Quat qz = from_axis_angle(Vec3{0, 0, 1}, yaw);
  return multiply(qz, multiply(qy, qx));
}

// A rotation chosen evenly (Shoemake's method): three numbers evenly in [0, 1), and two angles and two radii that
// together place a point evenly on the surface of a four dimensional sphere, which is where unit quaternions live.
inline Quat random_rotation(rc::core::Rng& rng) {
  const double u1 = rng.uniform();
  const double u2 = rng.uniform();
  const double u3 = rng.uniform();
  const double a = std::sqrt(1.0 - u1);
  const double b = std::sqrt(u1);
  return Quat{b * std::cos(kTwoPi * u3), a * std::sin(kTwoPi * u2), a * std::cos(kTwoPi * u2), b * std::sin(kTwoPi * u3)};
}

// How far the rotation turns, as the angle of the turn about its axis, from 0 to pi.
inline double rotation_angle(const Quat& q) {
  double w = std::fabs(q.w);
  if (w > 1.0) w = 1.0;
  return 2.0 * std::acos(w);
}

// The angle between two directions, in degrees.
inline double angle_between_degrees(const Vec3& a, const Vec3& b) {
  double c = a.x * b.x + a.y * b.y + a.z * b.z;
  if (c > 1.0) c = 1.0;
  if (c < -1.0) c = -1.0;
  return std::acos(c) * 180.0 / 3.14159265358979323846;
}

}  // namespace math
}  // namespace rc

#endif  // RC_MATH_RANDOM_ROTATION_HPP
