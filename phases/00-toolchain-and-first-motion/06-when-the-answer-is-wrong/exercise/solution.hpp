#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cmath>

#include <rc/sim/diff_drive.hpp>

// Six small questions you can ask about a pose. The answers are what turn "the
// robot ended up somewhere odd" into "the heading is wrong and the position is
// fine", which is a different sentence and a much shorter search.
//
// Write them, then read the tests at the bottom of the suite: they run a real
// trajectory and ask these questions of it, and then run three broken ones and
// require your answers to notice.

// How far apart two poses are, ignoring which way each is facing.
inline double distance_between(const rc::sim::Pose& a, const rc::sim::Pose& b) {
  // TODO: how far apart the two poses are, by Pythagoras.
  (void)a;
  (void)b;
  return 0.0;
}

// The shortest turn from one heading to another, signed, in radians.
//
// Not a subtraction. Angles wrap, so 179 degrees and minus 179 degrees are two
// degrees apart and subtracting says 358. Taking the sine and cosine of the
// difference and asking atan2 for the angle back throws the extra turns away,
// which is the same trick wrap_angle uses.
inline double heading_difference(double from, double to) {
  // TODO: the shortest turn from one heading to the other, signed.
  //
  // Not a subtraction. Angles wrap, so 179 degrees and minus 179 degrees are
  // two degrees apart and subtracting says 358. Take the sine and the cosine of
  // the difference and ask atan2 for the angle back: that throws away the extra
  // whole turns, and it is the same trick wrap_angle uses.
  (void)from;
  (void)to;
  return 0.0;
}

// Comparisons take a tolerance because these are fractional numbers. Two
// calculations that should agree exactly will differ in the last few bits, so
// asking whether they are equal gets the answer no for reasons that have
// nothing to do with the robot.
inline bool same_position(const rc::sim::Pose& a, const rc::sim::Pose& b, double tolerance) {
  // TODO: within the tolerance, not exactly equal.
  (void)a;
  (void)b;
  (void)tolerance;
  return false;
}

inline bool same_heading(double a, double b, double tolerance) {
  // TODO: within the tolerance. The difference has a sign and the size of it
  // is what matters, so a heading that is early and one that is late are both
  // wrong by the same amount.
  (void)a;
  (void)b;
  (void)tolerance;
  return false;
}

inline bool same_pose(const rc::sim::Pose& a, const rc::sim::Pose& b, double tolerance) {
  // TODO: the same place and the same direction.
  (void)a;
  (void)b;
  (void)tolerance;
  return false;
}

// Whether the robot covered the distance its speed and the elapsed time imply.
//
// This is the check that catches a factor. A model that averages when it should
// not, or applies dt twice, gives a trajectory that looks entirely plausible
// and is the wrong size, and nothing about the shape of it says so.
inline bool moved_expected_distance(const rc::sim::Pose& before, const rc::sim::Pose& after,
                                    double speed, double seconds, double tolerance) {
  // TODO: whether the distance covered is what the speed and the time imply.
  //
  // A distance is never negative, and driving backwards still covers ground.
  (void)before;
  (void)after;
  (void)speed;
  (void)seconds;
  (void)tolerance;
  return false;
}

#endif  // LESSON_SOLUTION_HPP
