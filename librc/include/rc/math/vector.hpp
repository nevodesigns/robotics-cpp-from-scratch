// rc/math/vector.hpp
//
// The vectors from lesson 06-01, graduated.
//
// Two and three dimensional displacements, with the two products a robot
// actually uses. The dot product answers how much two directions agree, which is
// what tells you whether a target is ahead or behind. The cross product answers
// which way one is turned from the other, which is what makes a cross track
// error signed and therefore steerable.

#ifndef RC_MATH_VECTOR_HPP
#define RC_MATH_VECTOR_HPP

#include <cmath>

namespace rc {
namespace math {

struct Vec2 {
  double x = 0.0;
  double y = 0.0;
};

struct Vec3 {
  double x = 0.0;
  double y = 0.0;
  double z = 0.0;
};

inline Vec2 add(const Vec2& a, const Vec2& b) { return Vec2{a.x + b.x, a.y + b.y}; }
inline Vec2 subtract(const Vec2& a, const Vec2& b) { return Vec2{a.x - b.x, a.y - b.y}; }
inline Vec2 scale(const Vec2& v, double by) { return Vec2{v.x * by, v.y * by}; }

inline Vec3 add(const Vec3& a, const Vec3& b) {
  return Vec3{a.x + b.x, a.y + b.y, a.z + b.z};
}
inline Vec3 subtract(const Vec3& a, const Vec3& b) {
  return Vec3{a.x - b.x, a.y - b.y, a.z - b.z};
}
inline Vec3 scale(const Vec3& v, double by) { return Vec3{v.x * by, v.y * by, v.z * by}; }

// hypot rather than sqrt of a sum of squares: the same answer without
// overflowing on very large components or losing very small ones.
inline double length(const Vec2& v) { return std::hypot(v.x, v.y); }
inline double length(const Vec3& v) { return std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z); }

inline double dot(const Vec2& a, const Vec2& b) { return a.x * b.x + a.y * b.y; }
inline double dot(const Vec3& a, const Vec3& b) { return a.x * b.x + a.y * b.y + a.z * b.z; }

// In two dimensions the cross product collapses to one signed number, the area
// of the parallelogram the vectors span. Its sign is the which side answer.
inline double cross(const Vec2& a, const Vec2& b) { return a.x * b.y - a.y * b.x; }

inline Vec3 cross(const Vec3& a, const Vec3& b) {
  return Vec3{a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}

// A vector of no length has no direction, so there is nothing to return but
// itself. Dividing would fill the result with NaN, which then spreads silently.
inline Vec2 normalized(const Vec2& v) {
  const double len = length(v);
  return len == 0.0 ? Vec2{} : scale(v, 1.0 / len);
}

inline Vec3 normalized(const Vec3& v) {
  const double len = length(v);
  return len == 0.0 ? Vec3{} : scale(v, 1.0 / len);
}

// atan2 of the cross and the dot, never acos of the normalised dot. acos loses
// small angles entirely, can leave its domain and return NaN, and has no sign.
inline double angle_between(const Vec2& a, const Vec2& b) {
  return std::atan2(cross(a, b), dot(a, b));
}

// Signed perpendicular distance from the line through from and to. Positive
// means the point is to the left, with x forward and y left. A segment of no
// length has no side, so it answers zero.
inline double cross_track_error(const Vec2& from, const Vec2& to, const Vec2& point) {
  const Vec2 along = subtract(to, from);
  const double base = length(along);
  if (base == 0.0) return 0.0;
  return cross(along, subtract(point, from)) / base;
}

}  // namespace math
}  // namespace rc

#endif  // RC_MATH_VECTOR_HPP
