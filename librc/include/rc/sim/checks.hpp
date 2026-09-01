// rc/sim/checks.hpp
//
// The questions from lesson 00-06, graduated.
//
// Six things you can ask about a pose, so that "the robot ended up somewhere
// odd" becomes "the heading is wrong and the position is fine", which is a
// different sentence and a much shorter search.
//
// Two of them exist because the obvious version reports a bug that is not
// there. Fractional numbers are never exactly equal, so every comparison takes
// a tolerance. And a heading of pi and a heading of minus pi point the same
// way, so a difference of headings goes through atan2 rather than through a
// subtraction.

#ifndef RC_SIM_CHECKS_HPP
#define RC_SIM_CHECKS_HPP

#include <cmath>

#include <rc/sim/diff_drive.hpp>

namespace rc {
namespace sim {

// How far apart two poses are, ignoring which way each is facing.
inline double distance_between(const Pose& a, const Pose& b) {
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
inline bool same_position(const Pose& a, const Pose& b, double tolerance) {
  return distance_between(a, b) <= tolerance;
}

inline bool same_heading(double a, double b, double tolerance) {
  return std::fabs(heading_difference(a, b)) <= tolerance;
}

inline bool same_pose(const Pose& a, const Pose& b, double tolerance) {
  return same_position(a, b, tolerance) && same_heading(a.theta, b.theta, tolerance);
}

// Whether the robot covered the distance its speed and the elapsed time imply.
//
// This is the check that catches a factor. A model that averages when it should
// not, or applies dt twice, gives a trajectory that looks entirely plausible
// and is the wrong size, and nothing about the shape of it says so.
inline bool moved_expected_distance(const Pose& before, const Pose& after,
                                    double speed, double seconds, double tolerance) {
  const double expected = std::fabs(speed * seconds);
  return std::fabs(distance_between(before, after) - expected) <= tolerance;
}

}  // namespace sim
}  // namespace rc

#endif  // RC_SIM_CHECKS_HPP
