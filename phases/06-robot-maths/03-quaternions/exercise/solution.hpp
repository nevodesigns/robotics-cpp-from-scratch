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
  // TODO
  //
  //   w = cos(angle / 2)
  //   the vector part is the unit axis scaled by sin(angle / 2)
  //
  // Halve the angle. Forgetting to is the most common quaternion bug there is,
  // and it gives a rotation of twice what was asked for. An axis of no length
  // describes no rotation, so answer the identity rather than dividing by zero.
  (void)axis; (void)angle;
  return Quat{};
}

inline Quat multiply(const Quat& a, const Quat& b) {
  // TODO
  //
  //   w = w1*w2 - v1 . v2
  //   v = w1*v2 + w2*v1 + v1 x v2
  //
  // Written out in components that is four lines. Both products from lesson
  // 06-01 appear in it, which is not a coincidence.
  (void)a; (void)b;
  return Quat{};
}

// For a unit quaternion this is the inverse, the same bargain the transpose gave
// for a rotation matrix and for the same reason.
inline Quat conjugate(const Quat& q) {
  // TODO: keep w, negate the rest. For a unit quaternion this is the inverse.
  (void)q;
  return Quat{};
}

inline double norm(const Quat& q) {
  // TODO
  (void)q;
  return 0.0;
}

inline Quat normalized(const Quat& q) {
  // TODO: divide by the norm. This is the whole repair for drift, against
  // orthogonalising three columns for a matrix.
  (void)q;
  return Quat{};
}

inline Vec3 rotate(const Quat& q, const Vec3& v) {
  // TODO: q * (0, v) * conjugate(q), then take the vector part.
  (void)q; (void)v;
  return Vec3{};
}

// Equality has to allow for the sign. Negating all four numbers gives a
// different quaternion describing exactly the same orientation, and code that
// ignores that works perfectly until an orientation crosses the boundary.
inline bool same_rotation(const Quat& a, const Quat& b, double tolerance) {
  // TODO: the same orientation when the four numbers match, or when every one
  // of them is the negation. Negating all four gives a different quaternion
  // describing exactly the same rotation, and forgetting that works perfectly
  // until an orientation crosses the boundary.
  (void)a; (void)b; (void)tolerance;
  return false;
}

#endif  // LESSON_SOLUTION_HPP
