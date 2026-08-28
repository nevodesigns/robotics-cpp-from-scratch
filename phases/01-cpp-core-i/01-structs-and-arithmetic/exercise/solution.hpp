// Grouping numbers that belong together.

#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cmath>

struct Pose {
  double x = 0.0;
  double y = 0.0;
  double theta = 0.0;
};

// Straight line distance between the two positions, in metres. Heading is
// ignored: two robots on the same spot facing opposite ways are zero apart.
inline double distance(const Pose& a, const Pose& b) {
  // TODO: Pythagoras. std::sqrt and std::pow are both available, and plain
  // multiplication works just as well as pow for squaring.
  (void)a;
  (void)b;
  return 0.0;
}

// The pose halfway between the two positions, with a heading of zero.
inline Pose midpoint(const Pose& a, const Pose& b) {
  // TODO: average each coordinate.
  (void)a;
  (void)b;
  return Pose{};
}

// The pose reached by driving forward metres along the heading start already
// faces. The heading does not change.
inline Pose translate(const Pose& start, double forward) {
  // TODO: cosine gives the x part of a direction, sine gives the y part.
  (void)forward;
  return start;
}

#endif  // LESSON_SOLUTION_HPP
