#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cmath>

#include <rc/core/compat.hpp>
#include <rc/kin/planar_arm.hpp>

// One full turn. Named, because the whole lesson turns on the difference
// between a number and an angle, and 6.28 appearing bare in a clamp is exactly
// how that difference gets lost.
const double kTurn = 6.283185307179586;

// How far one joint can turn. Not a preference: a hard stop, a cable that will
// not wind further, an arm that would otherwise fold into its own base.
struct Limit {
  double lo = 0.0;
  double hi = 0.0;

  double span() const { return hi - lo; }
};

// Bring an angle to its equivalent inside [lo, lo + one turn).
//
// An angle is a direction, not a quantity. -3.0 and 3.28 point the same way,
// and a joint that can hold one can hold the other, so the comparison has to be
// made after the turns are removed rather than before.
inline double at_same_direction(const Limit& limit, double angle) {
  double folded = limit.lo + std::fmod(angle - limit.lo, kTurn);
  if (folded < limit.lo) folded += kTurn;
  return folded;
}

// Can the joint hold this angle.
inline bool holds(const Limit& limit, double angle) {
  if (limit.span() >= kTurn) return true;
  return at_same_direction(limit, angle) <= limit.hi;
}

// Clamping a number into a range. Correct for a voltage, a duty cycle, a
// setpoint in metres, and wrong for an angle, which is why it is here under a
// name that says what it does rather than what it is used for.
inline double clamp_number(const Limit& limit, double value) {
  if (value < limit.lo) return limit.lo;
  if (value > limit.hi) return limit.hi;
  return value;
}

// The signed difference between two angles, brought into [-pi, pi]. The
// question "how far apart are these two directions" has no answer without it.
inline double angle_between(double from, double to) {
  double difference = std::fmod(to - from + kTurn / 2.0, kTurn);
  if (difference < 0.0) difference += kTurn;
  return difference - kTurn / 2.0;
}

// Clamping an angle into a range: the legal direction closest to the one asked
// for. When the direction is legal at some turn, that is the answer. When it is
// not, the answer is whichever end is nearer measured the way angles are
// measured, which is not always the end a number comparison would pick.
inline double clamp_angle(const Limit& limit, double angle) {
  if (limit.span() >= kTurn) return angle;

  const double folded = at_same_direction(limit, angle);
  if (folded <= limit.hi) return folded;

  const double to_low = std::fabs(angle_between(folded, limit.lo));
  const double to_high = std::fabs(angle_between(folded, limit.hi));
  return to_low <= to_high ? limit.lo : limit.hi;
}

// Both joints of the planar arm.
struct ArmLimits {
  Limit shoulder;
  Limit elbow;
};

inline bool holds(const ArmLimits& limits, const rc::kin::ArmSolution& solution) {
  return holds(limits.shoulder, solution.q1) && holds(limits.elbow, solution.q2);
}

// Which of the two answers from lesson 13-02 this arm can actually take up.
//
// Reporting both flags rather than one solution is the point. A target the arm
// can reach with one elbow and not the other is an ordinary target, not an edge
// case, and code that solves for one branch and stops calls it unreachable.
struct LegalSolutions {
  rc::kin::ArmSolution elbow_up;
  rc::kin::ArmSolution elbow_down;
  bool up_holds = false;
  bool down_holds = false;

  bool any() const { return up_holds || down_holds; }
  int count() const { return (up_holds ? 1 : 0) + (down_holds ? 1 : 0); }
};

inline rc::expected<LegalSolutions, rc::kin::ReachError> legal_solutions(
    double x, double y, double l1, double l2, const ArmLimits& limits) {
  const auto solved = rc::kin::solve(x, y, l1, l2);
  if (!solved) return rc::unexpected(solved.error());

  const rc::kin::Solutions& both = solved.value();

  LegalSolutions legal;
  legal.elbow_up = both.elbow_up;
  legal.elbow_down = both.elbow_down;
  legal.up_holds = holds(limits, legal.elbow_up);
  legal.down_holds = holds(limits, legal.elbow_down);
  return legal;
}

// Where the tool ends up for a given pair of joint angles.
struct ToolPoint {
  double x = 0.0;
  double y = 0.0;
};

inline ToolPoint tool_at(double l1, double l2, double q1, double q2) {
  return ToolPoint{l1 * std::cos(q1) + l2 * std::cos(q1 + q2),
                   l1 * std::sin(q1) + l2 * std::sin(q1 + q2)};
}

// A pose the arm can hold, and how far it lands from where you wanted.
struct NearestPose {
  double q1 = 0.0;
  double q2 = 0.0;
  ToolPoint tool;
  double distance = 0.0;
  int evaluations = 0;
};

// The closest the arm can get, given its limits.
//
// This is a search over two joints, and it does not have to be. For a fixed
// elbow the tool sits at a fixed radius from the base, and the shoulder only
// turns that radius to a bearing, so the best shoulder angle for any elbow is
// the target's bearing minus the arm's own offset, clamped as an angle. That
// leaves one variable to scan instead of two, which is the difference between a
// few hundred evaluations and a few million.
inline NearestPose nearest_reachable(double x, double y, double l1, double l2,
                                     const ArmLimits& limits, int elbow_steps) {
  if (elbow_steps < 1) elbow_steps = 1;

  const double bearing = std::atan2(y, x);
  NearestPose best;
  best.distance = -1.0;

  for (int step = 0; step <= elbow_steps; ++step) {
    const double fraction = static_cast<double>(step) / static_cast<double>(elbow_steps);
    const double q2 = limits.elbow.lo + limits.elbow.span() * fraction;

    // The angle between the first link and the line from base to tool. Take it
    // off the bearing and the arm points where it should.
    const double offset = std::atan2(l2 * std::sin(q2), l1 + l2 * std::cos(q2));
    const double q1 = clamp_angle(limits.shoulder, bearing - offset);

    const ToolPoint reached = tool_at(l1, l2, q1, q2);
    const double distance = std::hypot(reached.x - x, reached.y - y);
    ++best.evaluations;

    if (best.distance < 0.0 || distance < best.distance) {
      best.q1 = q1;
      best.q2 = q2;
      best.tool = reached;
      best.distance = distance;
    }
  }

  return best;
}

#endif  // LESSON_SOLUTION_HPP
