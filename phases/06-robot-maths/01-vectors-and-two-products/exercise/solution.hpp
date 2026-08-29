#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cmath>

// A displacement: how far across and how far up. It says nothing about where it
// starts, which is why subtracting two positions gives one.
struct Vec2 {
  double x = 0.0;
  double y = 0.0;
};

inline Vec2 add(const Vec2& a, const Vec2& b) {
  // TODO
  (void)a; (void)b; return Vec2{};
}

inline Vec2 subtract(const Vec2& a, const Vec2& b) {
  // TODO
  (void)a; (void)b; return Vec2{};
}

inline Vec2 scale(const Vec2& v, double by) {
  // TODO
  (void)v; (void)by; return Vec2{};
}

inline double length(const Vec2& v) {
  // TODO: std::hypot gives the same answer as sqrt(x*x + y*y) without
  // overflowing on very large components or losing very small ones.
  (void)v;
  return 0.0;
}

// How much the two agree. Positive when they point the same way, zero when
// perpendicular, negative when opposed. That sign alone answers whether a target
// is in front of or behind a heading.
inline double dot(const Vec2& a, const Vec2& b) {
  // TODO
  (void)a; (void)b; return 0.0;
}

// How much b is turned from a. Zero when parallel, positive when b is
// anticlockwise from a, negative when clockwise. In two dimensions this is one
// number, the signed area of the parallelogram the two vectors span.
inline double cross(const Vec2& a, const Vec2& b) {
  // TODO: in two dimensions this is one number. Mind the order of the terms:
  // swapping them negates the result, and the sign is the whole point.
  (void)a; (void)b; return 0.0;
}

inline Vec2 normalized(const Vec2& v) {
  // TODO: the same direction with length one. A zero vector has no direction,
  // so decide what to return rather than dividing and filling it with NaN.
  (void)v;
  return Vec2{};
}

inline double angle_between(const Vec2& a, const Vec2& b) {
  // TODO: std::atan2 of the cross and the dot, not acos of the normalised dot.
  // The reason is in docs/en.md and it is the point of this lesson.
  (void)a; (void)b; return 0.0;
}

inline double cross_track_error(const Vec2& from, const Vec2& to, const Vec2& point) {
  // TODO
  //
  // The cross product of the path direction and the offset to the robot is the
  // area of the parallelogram they span. Dividing by the path length turns that
  // area into a height, which is the perpendicular distance, and the sign says
  // which side.
  //
  // Positive means the robot is to the left. A segment with no length has no
  // side, so answer 0.0 rather than dividing by zero.
  (void)from; (void)to; (void)point;
  return 0.0;
}

#endif  // LESSON_SOLUTION_HPP
