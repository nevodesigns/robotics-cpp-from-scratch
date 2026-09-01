#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cmath>

#include <rc/sim/diff_drive.hpp>

// Six small questions you can ask about a pose, and the answers are what turn
// "the robot ended up somewhere odd" into "the heading is wrong and the
// position is fine", which is a different sentence and a much shorter search.

// How far apart two poses are, ignoring which way each is facing.
inline double distance_between(const rc::sim::Pose& a, const rc::sim::Pose& b) {
  const double dx = a.x - b.x;
  const double dy = a.y - b.y;
  return std::sqrt(dx * dx + dy * dy);
}

// The shortest turn from one heading to another, signed, in radians.
//
// Not a subtraction. Angles wrap, so 179 degrees and minus 179 degrees are two
// degrees apart and subtracting says 358. Taking the sine and cosine of the
// difference and asking atan2 for the angle back throws the extra turns away,
// which is the same trick wrap_angle uses.
inline double heading_difference(double from, double to) {
  const double difference = to - from;
  return std::atan2(std::sin(difference), std::cos(difference));
}

// Comparisons take a tolerance because these are fractional numbers. Two
// calculations that should agree exactly will differ in the last few bits, so
// asking whether they are equal gets the answer no for reasons that have
// nothing to do with the robot.
inline bool same_position(const rc::sim::Pose& a, const rc::sim::Pose& b, double tolerance) {
  return distance_between(a, b) <= tolerance;
}

inline bool same_heading(double a, double b, double tolerance) {
  return std::fabs(heading_difference(a, b)) <= tolerance;
}

inline bool same_pose(const rc::sim::Pose& a, const rc::sim::Pose& b, double tolerance) {
  return same_position(a, b, tolerance) && same_heading(a.theta, b.theta, tolerance);
}

// Whether the robot covered the distance its speed and the elapsed time imply.
//
// This is the check that catches a factor. A model that averages when it should
// not, or applies dt twice, gives a trajectory that looks entirely plausible
// and is the wrong size, and nothing about the shape of it says so.
inline bool moved_expected_distance(const rc::sim::Pose& before, const rc::sim::Pose& after,
                                    double speed, double seconds, double tolerance) {
  const double expected = std::fabs(speed * seconds);
  return std::fabs(distance_between(before, after) - expected) <= tolerance;
}

#endif  // LESSON_SOLUTION_HPP
