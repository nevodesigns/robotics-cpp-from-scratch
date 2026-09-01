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
  solution.q2 = q2;
  solution.q1 = std::atan2(y, x) -
                std::atan2(l2 * std::sin(q2), l1 + l2 * std::cos(q2));
  return solution;
}

// The closed form. Two links, one plane, and an answer that can be written
// down rather than searched for.
inline rc::expected<Solutions, ReachError> solve(double x, double y, double l1, double l2) {
  const double reach = std::hypot(x, y);
  const double furthest = l1 + l2;
  const double nearest = std::fabs(l1 - l2);

  // A tolerance on both, because a target computed rather than typed lands on
  // the boundary a fraction outside it, and refusing a point the arm can very
  // nearly touch is its own kind of wrong.
  const double tolerance = 1e-9;
  if (reach > furthest + tolerance) return rc::unexpected(ReachError::TooFar);
  if (reach < nearest - tolerance) return rc::unexpected(ReachError::TooClose);

  // The law of cosines, rearranged for the elbow.
  double cos_q2 = (reach * reach - l1 * l1 - l2 * l2) / (2.0 * l1 * l2);

  // The clamp is not defensive noise. Reaching the edge of the workspace is an
  // ordinary thing for an arm to do, and a target computed from the arm's own
  // forward kinematics at full extension gives an argument of about 1 + 7e-16.
  // Measured across a hundred thousand fully extended configurations, more than
  // half produced an argument above one, and acos of anything above one is not
  // a number, which then spreads into every joint command that follows.
  if (cos_q2 > 1.0) cos_q2 = 1.0;
  if (cos_q2 < -1.0) cos_q2 = -1.0;

  // acos answers between zero and pi, so it gives one of the two elbows. The
  // other is its negation, which is the whole reason there are two.
  const double bend = std::acos(cos_q2);

  Solutions solutions;
  solutions.elbow_down = with_elbow(x, y, l1, l2, bend);
  solutions.elbow_up = with_elbow(x, y, l1, l2, -bend);

  // sin(q2) near zero means the arm is straight out or folded back, and the two
  // answers have met.
  solutions.coincide = std::fabs(std::sin(bend)) < 1e-9;
  return solutions;
}

#endif  // LESSON_SOLUTION_HPP
