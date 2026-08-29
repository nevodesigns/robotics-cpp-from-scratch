#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cmath>

// A displacement: how far across and how far up. It says nothing about where it
// starts, which is why subtracting two positions gives one.
struct Vec2 {
  double x = 0.0;
  double y = 0.0;
};

inline Vec2 add(const Vec2& a, const Vec2& b) { return Vec2{a.x + b.x, a.y + b.y}; }
inline Vec2 subtract(const Vec2& a, const Vec2& b) { return Vec2{a.x - b.x, a.y - b.y}; }
inline Vec2 scale(const Vec2& v, double by) { return Vec2{v.x * by, v.y * by}; }

inline double length(const Vec2& v) {
  // hypot rather than sqrt(x*x + y*y): the same answer, without overflowing when
  // the components are very large or losing the small ones when they are tiny.
  return std::hypot(v.x, v.y);
}

// How much the two agree. Positive when they point the same way, zero when
// perpendicular, negative when opposed. That sign alone answers whether a target
// is in front of or behind a heading.
inline double dot(const Vec2& a, const Vec2& b) { return a.x * b.x + a.y * b.y; }

// How much b is turned from a. Zero when parallel, positive when b is
// anticlockwise from a, negative when clockwise. In two dimensions this is one
// number, the signed area of the parallelogram the two vectors span.
inline double cross(const Vec2& a, const Vec2& b) { return a.x * b.y - a.y * b.x; }

inline Vec2 normalized(const Vec2& v) {
  const double len = length(v);

  // A zero vector has no direction, so there is nothing to return but itself.
  // Dividing here would fill the result with NaN, which then spreads silently
  // through every later calculation.
  if (len == 0.0) return Vec2{0.0, 0.0};

  return scale(v, 1.0 / len);
}

inline double angle_between(const Vec2& a, const Vec2& b) {
  // atan2 of the cross and the dot rather than acos of the normalised dot.
  //
  // acos is flat near zero, so it turns a tiny error into a large one exactly
  // where a path follower lives; rounding can push its argument past one, which
  // gives NaN; and it has no sign, so it cannot tell left from right. atan2 has
  // none of those faults and answers in minus pi to pi already.
  return std::atan2(cross(a, b), dot(a, b));
}

inline double cross_track_error(const Vec2& from, const Vec2& to, const Vec2& point) {
  const Vec2 along = subtract(to, from);
  const double base = length(along);

  // A segment of zero length has no direction, so there is no side to be on.
  if (base == 0.0) return 0.0;

  const Vec2 offset = subtract(point, from);

  // The cross product is the area of the parallelogram those two span. Dividing
  // by the base turns an area into a height, which is the perpendicular
  // distance, and the sign survives to say which side.
  //
  // With x forward and y left, positive means the robot is to the left of the
  // path. That convention has to be written down once and never changed.
  return cross(along, offset) / base;
}

#endif  // LESSON_SOLUTION_HPP
