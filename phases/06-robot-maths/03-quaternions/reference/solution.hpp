#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cmath>

struct Vec3 {
  double x = 0.0;
  double y = 0.0;
  double z = 0.0;
};

// A unit quaternion: the scalar part records how far around, and the vector part
// points along the axis. The identity is no rotation at all.
struct Quat {
  double w = 1.0;
  double x = 0.0;
  double y = 0.0;
  double z = 0.0;
};

inline double length(const Vec3& v) { return std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z); }

inline Quat from_axis_angle(const Vec3& axis, double angle) {
  const double axis_length = length(axis);

  // An axis of no length describes no rotation. Dividing here would fill all
  // four numbers with NaN, which then spreads through every pose downstream.
  if (axis_length == 0.0) return Quat{};

  // The half angle. Every quaternion bug that is not a sign error is somebody
  // forgetting this, and the symptom is a rotation of twice or half what was
  // intended.
  const double half = angle / 2.0;
  const double s = std::sin(half) / axis_length;

  return Quat{std::cos(half), axis.x * s, axis.y * s, axis.z * s};
}

inline Quat multiply(const Quat& a, const Quat& b) {
  // The scalar part carries how much the two agree, which is a dot product, and
  // the vector part carries the turning between them, which is a cross product.
  // Both products from lesson 06-01, and not by coincidence.
  return Quat{
      a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z,
      a.w * b.x + a.x * b.w + a.y * b.z - a.z * b.y,
      a.w * b.y - a.x * b.z + a.y * b.w + a.z * b.x,
      a.w * b.z + a.x * b.y - a.y * b.x + a.z * b.w,
  };
}

// For a unit quaternion this is the inverse, the same bargain the transpose gave
// for a rotation matrix and for the same reason.
inline Quat conjugate(const Quat& q) { return Quat{q.w, -q.x, -q.y, -q.z}; }

inline double norm(const Quat& q) {
  return std::sqrt(q.w * q.w + q.x * q.x + q.y * q.y + q.z * q.z);
}

inline Quat normalized(const Quat& q) {
  const double n = norm(q);
  if (n == 0.0) return Quat{};   // nothing to point at, so the identity

  // The whole repair: one square root and four divisions, exact. A rotation
  // matrix needs three columns orthogonalised to achieve the same thing.
  return Quat{q.w / n, q.x / n, q.y / n, q.z / n};
}

inline Vec3 rotate(const Quat& q, const Vec3& v) {
  const Quat pure{0.0, v.x, v.y, v.z};
  const Quat turned = multiply(multiply(q, pure), conjugate(q));
  return Vec3{turned.x, turned.y, turned.z};
}

// Equality has to allow for the sign. Negating all four numbers gives a
// different quaternion describing exactly the same orientation, and code that
// ignores that works perfectly until an orientation crosses the boundary.
inline bool same_rotation(const Quat& a, const Quat& b, double tolerance) {
  const bool identical = std::fabs(a.w - b.w) <= tolerance && std::fabs(a.x - b.x) <= tolerance &&
                         std::fabs(a.y - b.y) <= tolerance && std::fabs(a.z - b.z) <= tolerance;
  const bool negated = std::fabs(a.w + b.w) <= tolerance && std::fabs(a.x + b.x) <= tolerance &&
                       std::fabs(a.y + b.y) <= tolerance && std::fabs(a.z + b.z) <= tolerance;
  return identical || negated;
}

#endif  // LESSON_SOLUTION_HPP
