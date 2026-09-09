// rc/kin/limits.hpp
//
// Joint limits, from lesson 13-05, graduated.
//
// rc/kin/planar_arm.hpp answers where the joints have to be. This answers
// whether the arm can get there, which is a different question and is not
// answered by the geometry. A stop, a cable, a link that would fold into the
// base: the reachable set is smaller than the workspace and the part it loses
// is not all at the edge.
//
// Three numbers from that lesson are worth carrying with the code. For a
// 0.5 m and 0.4 m arm with the shoulder held to [-0.6, 2.2] and the elbow to
// [-2.4, 2.4], measured on a 1 cm grid: 55.2 percent of the geometric
// workspace survives the limits, and a solver that computes one elbow branch
// and stops finds only 70.1 percent of what survives. On targets where the
// branch it happened to pick was the illegal one, clamping that branch's
// angles landed the tool 0.45 to 0.73 m from the target while the other branch
// was exact. Clamping is not a repair.
//
// And nearest_reachable is a one dimensional search where the obvious
// implementation is two dimensional. Against a 601 by 601 grid over both
// joints it was never worse, on eight targets, using 16008 evaluations against
// 2889608.

#ifndef RC_KIN_LIMITS_HPP
#define RC_KIN_LIMITS_HPP

#include <cmath>

#include <rc/core/compat.hpp>
#include <rc/kin/planar_arm.hpp>

namespace rc {
namespace kin {

// One full turn. Named, because the difference between a number and an angle
// is the whole of this header, and a bare 6.28 in a clamp is how it gets lost.
constexpr double kTurn = 6.283185307179586;

// How far one joint can turn, and no further.
struct Limit {
  double lo = 0.0;
  double hi = 0.0;

  double span() const { return hi - lo; }
};

// The equivalent angle inside [lo, lo + one turn).
//
// An angle is a direction, not a quantity. -3.0 and 3.28 point the same way, so
// a joint that can hold one holds the other, and the comparison has to happen
// after the turns come off rather than before. solve() builds q1 from a
// difference of two atan2 calls and hands back angles across roughly
// (-2pi, 2pi), so this is the ordinary case and not a guard against nonsense.
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

// The signed difference between two directions, in [-pi, pi]. "How far apart
// are these two angles" has no answer without it.
inline double angle_between(double from, double to) {
  double difference = std::fmod(to - from + kTurn / 2.0, kTurn);
  if (difference < 0.0) difference += kTurn;
  return difference - kTurn / 2.0;
}

// Clamping a number into a range. Right for a voltage, a duty cycle, a
// setpoint in metres. Wrong for an angle, and named for what it does rather
// than what it tends to get used for.
inline double clamp_number(const Limit& limit, double value) {
  if (value < limit.lo) return limit.lo;
  if (value > limit.hi) return limit.hi;
  return value;
}

// Clamping an angle: the legal direction closest to the one asked for.
//
// Measured on a shoulder limited to [-0.6, 2.2], across six illegal
// directions, clamp_number picked the further end three times. Asked for
// -3.0 rad it answers -0.6, which is 2.4 rad from where you wanted to point,
// when 2.2 is 1.08 rad away.
inline double clamp_angle(const Limit& limit, double angle) {
  if (limit.span() >= kTurn) return angle;

  const double folded = at_same_direction(limit, angle);
  if (folded <= limit.hi) return folded;

  const double to_low = std::fabs(angle_between(folded, limit.lo));
  const double to_high = std::fabs(angle_between(folded, limit.hi));
  return to_low <= to_high ? limit.lo : limit.hi;
}

// Both joints of a planar arm.
struct ArmLimits {
  Limit shoulder;
  Limit elbow;
};

inline bool holds(const ArmLimits& limits, const ArmSolution& solution) {
  return holds(limits.shoulder, solution.q1) && holds(limits.elbow, solution.q2);
}

// The two answers, each with a verdict.
//
// Both flags rather than one solution, for the same reason Solutions returns
// both elbows: a target the arm can reach with one elbow and not the other is
// an ordinary target, and code that solves one branch and stops calls it
// unreachable. On the arm measured above that is three cells in ten.
struct LegalSolutions {
  ArmSolution elbow_up;
  ArmSolution elbow_down;
  bool up_holds = false;
  bool down_holds = false;

  bool any() const { return up_holds || down_holds; }
  int count() const { return (up_holds ? 1 : 0) + (down_holds ? 1 : 0); }
};

// Solve, then ask the limits. The limits are a second filter and not a
// replacement for the first: a point beyond the links is still TooFar.
inline rc::expected<LegalSolutions, ReachError> legal_solutions(
    double x, double y, double l1, double l2, const ArmLimits& limits) {
  const auto solved = solve(x, y, l1, l2);
  if (!solved) return rc::unexpected(solved.error());

  const Solutions& both = solved.value();

  LegalSolutions legal;
  legal.elbow_up = both.elbow_up;
  legal.elbow_down = both.elbow_down;
  legal.up_holds = holds(limits, legal.elbow_up);
  legal.down_holds = holds(limits, legal.elbow_down);
  return legal;
}

// Where the tool ends up for a pair of joint angles.
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
// A search over two joints that does not have to be one. For a fixed elbow the
// tool sits at a fixed radius from the base and the shoulder only turns that
// radius to a bearing, so the best shoulder for any elbow is the target's
// bearing minus the arm's own offset, clamped as an angle. One variable is
// left to scan.
//
// elbow_steps buys accuracy and nothing else. Against a 601 by 601 grid on
// five targets: 8 steps trailed by 4.2e-02 m, 32 by 5.0e-03, 128 by 3.1e-03,
// 512 by 2.3e-06, and 2048 by nothing measurable. Pick it from the tolerance
// you need rather than inheriting it.
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
    // off the bearing and the whole arm points where it should.
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

}  // namespace kin
}  // namespace rc

#endif  // RC_KIN_LIMITS_HPP
