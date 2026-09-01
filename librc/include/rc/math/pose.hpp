// rc/math/pose.hpp
//
// The pose from lesson 01-01, graduated.
//
// Where the robot is and which way it faces, as one value rather than three
// loose numbers that can be passed in the wrong order.

#ifndef RC_MATH_POSE
#define RC_MATH_POSE

#include <cmath>

namespace rc {
namespace math {

struct Pose {
  double x = 0.0;
  double y = 0.0;
  double theta = 0.0;
};

inline double distance(const Pose& a, const Pose& b) {
  const double dx = b.x - a.x;
  const double dy = b.y - a.y;
  // std::hypot rather than sqrt(dx*dx + dy*dy). It gives the same answer and
  // avoids overflowing when the differences are very large, which costs nothing
  // and removes a failure mode.
  return std::hypot(dx, dy);
}

inline Pose midpoint(const Pose& a, const Pose& b) {
  Pose middle;
  middle.x = (a.x + b.x) / 2.0;
  middle.y = (a.y + b.y) / 2.0;
  // Averaging two headings is not as simple as averaging two numbers, because
  // angles wrap. Rather than get it subtly wrong, this function does not claim
  // to produce a heading at all. Lesson 06 handles averaging angles properly.
  middle.theta = 0.0;
  return middle;
}

inline Pose translate(const Pose& start, double forward) {
  Pose moved = start;
  moved.x = start.x + forward * std::cos(start.theta);
  moved.y = start.y + forward * std::sin(start.theta);
  return moved;
}

}  // namespace math
}  // namespace rc

#endif  // RC_MATH_POSE
