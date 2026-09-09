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

// TODO 1: can the joint hold this angle.
//
// Not "is the number between lo and hi". The solver in lesson 13-02 builds q1
// from a difference of two atan2 calls, so it hands back angles anywhere in
// roughly (-2pi, 2pi), and a joint that can sit at 1.0 can equally sit at
// 1.0 - 2pi. Bring the angle to the same direction inside the range first,
// using the helper above, and compare after that rather than before.
//
// A joint with a span of a full turn or more has no limit worth checking.
//
// The test asks whether 1.0, 1.0 + 2pi and 1.0 - 2pi are all held by a joint
// limited to [-0.6, 2.2], and whether 3.0 is not.
inline bool holds(const Limit& limit, double angle) {
  (void)limit;
  (void)angle;
  return true;
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
// TODO 2: the legal direction closest to the one asked for.
//
// Two cases. If the direction is legal at some turn, that turn is the answer
// and there is nothing to clamp. If it is not, the answer is one of the two
// ends, and which one is a question about angles: use angle_between, not a
// comparison of the numbers.
//
// This is where clamp_number above gets it wrong, and the test measures by how
// much. A shoulder limited to [-0.6, 2.2] asked for -3.0 rad: the number clamp
// says -0.6, which is 2.4 rad away from where you wanted to point, and 2.2 is
// 1.08 rad away.
inline double clamp_angle(const Limit& limit, double angle) {
  (void)limit;
  return angle;
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

// TODO 3: both answers, each with a verdict.
//
// Call rc::kin::solve for the two branches, and pass an unreachable target's
// error straight back out: the limits are a second filter and they do not
// replace the first, so a point beyond the links is still TooFar.
//
// Then check each branch against both joints. Do not pick one and return it.
// The caller knows which elbow keeps clear of the table and you do not, and the
// test measures what choosing for them costs.
inline rc::expected<LegalSolutions, rc::kin::ReachError> legal_solutions(
    double x, double y, double l1, double l2, const ArmLimits& limits) {
  (void)x;
  (void)y;
  (void)l1;
  (void)l2;
  (void)limits;
  return LegalSolutions{};
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
// TODO 4: the closest pose the arm can hold.
//
// Scan the elbow across its legal range in elbow_steps + 1 samples. For each
// one, work out the shoulder angle that points the arm at the target:
//
//   offset  = atan2(l2 sin(q2), l1 + l2 cos(q2))
//   shoulder = clamp_angle(limits.shoulder, atan2(y, x) - offset)
//
// That offset is the angle between the first link and the line from the base
// out to the tool, so taking it off the target's bearing aims the whole arm.
// Keep whichever sample lands nearest, and count every one you tried into
// evaluations, because the test compares that count against a search over both
// joints.
//
// Guard elbow_steps below one, since a caller who asks for zero samples should
// still get a pose back rather than a division by zero.
inline NearestPose nearest_reachable(double x, double y, double l1, double l2,
                                     const ArmLimits& limits, int elbow_steps) {
  (void)x;
  (void)y;
  (void)l1;
  (void)l2;
  (void)limits;
  (void)elbow_steps;
  return NearestPose{};
}

#endif  // LESSON_SOLUTION_HPP
