#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cmath>

#include <rc/core/compat.hpp>

// One way of holding the arm so that the tool lands on the target.
struct ArmSolution {
  double q1 = 0.0;   // shoulder
  double q2 = 0.0;   // elbow
};

// Both of them.
//
// A planar two link arm reaching a point almost always has exactly two answers,
// and the maths does not prefer either. Which to use is a decision about the
// world: which one avoids the table, which one the arm is nearer to now, which
// one keeps the elbow away from the operator. Returning one and not saying so
// makes that decision silently and wrongly half the time.
struct Solutions {
  ArmSolution elbow_up;
  ArmSolution elbow_down;

  // At the very edge of the workspace, and at the inner limit, the two answers
  // are the same answer. Worth reporting, because that is also where a small
  // move of the target demands a large move of the joints.
  bool coincide = false;
};

enum class ReachError {
  TooFar,     // beyond the sum of the links
  TooClose,   // inside the hole the shorter link cannot reach into
};

// Given the elbow angle, the shoulder angle that points the whole arm at the
// target. Two atan2 calls, and never a plain division: atan2 knows which
// quadrant the answer is in and a ratio does not.
inline ArmSolution with_elbow(double x, double y, double l1, double l2, double q2) {
  ArmSolution solution;
  // TODO: the shoulder angle that points the arm at the target, given this
  // elbow angle.
  //
  // Two atan2 calls, and never a plain division. atan2 knows which quadrant the
  // answer is in and a ratio does not, so a target behind the arm gets solved
  // as though it were in front.
  (void)x; (void)y; (void)l1; (void)l2; (void)q2;
  return solution;
}

// The closed form. Two links, one plane, and an answer that can be written
// down rather than searched for.
inline rc::expected<Solutions, ReachError> solve(double x, double y, double l1, double l2) {
  // TODO: both answers, or the reason there is none.
  //
  // Reachability first. Beyond the sum of the links is TooFar; inside the
  // difference between them is TooClose, the hole the arm cannot fold into.
  // Allow a tolerance on both, because a target computed rather than typed
  // lands on the boundary a fraction outside it, and refusing a point the arm
  // can very nearly touch is its own kind of wrong.
  //
  // Then the law of cosines, rearranged for the elbow:
  //
  //     cos(q2) = (r^2 - l1^2 - l2^2) / (2 * l1 * l2)
  //
  // Clamp that to minus one through one before handing it to acos, and this is
  // not defensive noise. Reaching the edge of the workspace is an ordinary
  // thing for an arm to do, and a target computed from the arm's own forward
  // kinematics at full extension gives an argument of about 1 + 7e-16.
  // Measured across a hundred thousand fully extended configurations, more than
  // half produced an argument above one, and acos of anything above one is not
  // a number.
  //
  // acos answers between zero and pi, which is one of the two elbows. The other
  // is its negation, and that is the whole reason there are two.
  //
  // Report whether they coincide, which happens when sin of the elbow angle is
  // nearly zero: the arm is straight out or folded back, and the two answers
  // have met.
  (void)x; (void)y; (void)l1; (void)l2;
  return Solutions{};
}

#endif  // LESSON_SOLUTION_HPP
