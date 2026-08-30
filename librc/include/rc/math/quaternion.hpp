// rc/math/quaternion.hpp
//
// The orientation type from lesson 06-03, graduated.
//
// Four numbers holding an axis and a half angle. No orientation at which the
// representation degenerates, and repair is one division by the norm rather
// than orthogonalising three columns. Measured over a hundred thousand
// compositions, a quaternion drifts about as much as a rotation matrix does;
// the advantage is the cost of putting it back.

#ifndef RC_MATH_QUATERNION_HPP
#define RC_MATH_QUATERNION_HPP

#include <cmath>

#include "rc/math/vector.hpp"

namespace rc {
namespace math {

struct Quat {
  double w = 1.0;
  double x = 0.0;
  double y = 0.0;
  double z = 0.0;
};

// The angle is halved. Forgetting that is the most common quaternion bug there
// is, and it gives a rotation of exactly twice what was asked for.
inline Quat from_axis_angle(const Vec3& axis, double angle) {
  const double axis_length = length(axis);
  if (axis_length == 0.0) return Quat{};

  const double half = angle / 2.0;
  const double s = std::sin(half) / axis_length;
  return Quat{std::cos(half), axis.x * s, axis.y * s, axis.z * s};
}

inline Quat multiply(const Quat& a, const Quat& b) {
  return Quat{
      a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z,
      a.w * b.x + a.x * b.w + a.y * b.z - a.z * b.y,
      a.w * b.y - a.x * b.z + a.y * b.w + a.z * b.x,
      a.w * b.z + a.x * b.y - a.y * b.x + a.z * b.w,
  };
}

// For a unit quaternion this is the inverse, the same bargain a transpose gives
// for a rotation matrix.
inline Quat conjugate(const Quat& q) { return Quat{q.w, -q.x, -q.y, -q.z}; }

inline double norm(const Quat& q) {
  return std::sqrt(q.w * q.w + q.x * q.x + q.y * q.y + q.z * q.z);
}

inline Quat normalized(const Quat& q) {
  const double n = norm(q);
  if (n == 0.0) return Quat{};
  return Quat{q.w / n, q.x / n, q.y / n, q.z / n};
}

inline Vec3 rotate(const Quat& q, const Vec3& v) {
  const Quat pure{0.0, v.x, v.y, v.z};
  const Quat turned = multiply(multiply(q, pure), conjugate(q));
  return Vec3{turned.x, turned.y, turned.z};
}

// Negating all four numbers gives a different quaternion describing the same
// rotation, so equality has to allow for the sign. Code that compares the
// numbers directly works until an orientation crosses the boundary.
inline bool same_rotation(const Quat& a, const Quat& b, double tolerance) {
  const bool identical =
      std::fabs(a.w - b.w) <= tolerance && std::fabs(a.x - b.x) <= tolerance &&
      std::fabs(a.y - b.y) <= tolerance && std::fabs(a.z - b.z) <= tolerance;
  const bool negated =
      std::fabs(a.w + b.w) <= tolerance && std::fabs(a.x + b.x) <= tolerance &&
      std::fabs(a.y + b.y) <= tolerance && std::fabs(a.z + b.z) <= tolerance;
  return identical || negated;
}

}  // namespace math
}  // namespace rc

#endif  // RC_MATH_QUATERNION_HPP
