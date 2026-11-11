#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cmath>
#include <limits>
#include <vector>
#include <rc/kin/arm3.hpp>
#include <rc/kin/limits.hpp>
#include <rc/kin/timing.hpp>

// Phase 13 ends with an arm that is asked its question in both directions, and told apart the pose it cannot reach from the one it cannot hold. Forward kinematics says where the tool is
// for angles; inverse says which angles put it somewhere. Neither says whether the arm can do it, and there are three different reasons that it cannot, with three different remedies:
//
//   out of reach         no angles at all put the tool there        (13-02, 13-11)     move the target
//   outside the limits   angles exist, and a joint cannot turn to them   (13-05)        another way of reaching it, or another target
//   too heavy            angles exist and are legal, and the motors cannot hold them  (13-06)   a lighter payload, or the other way of reaching it
//
// A planner that says only "failed" gives the operator nothing to change. This one says which, and for a pose that can be done, picks the way of doing it that takes the least time
// among those the joints can take and the motors can hold, and says how long.

using rc::kin::Arm3Solution;
using rc::kin::Pose2;

struct ArmSpec {
  double length[3] = {1.0, 1.0, 1.0};
  double mass[3] = {2.0, 1.5, 1.0};                    // each link, uniform, so its centre is half way along
  double payload = 0.0;                                // at the tool, kilograms
  double torque_limit[3] = {80.0, 40.0, 15.0};         // newton metres each motor can hold
  rc::kin::Limit angle_limit[3] = {{-3.14159265358979, 3.14159265358979}, {-2.8, 2.8}, {-2.8, 2.8}};
  double vmax[3] = {2.0, 2.0, 3.0};                    // radians a second
  double amax[3] = {6.0, 6.0, 10.0};                   // radians a second squared
  double gravity = 9.81;
};

enum class Verdict { kOk, kUnreachable, kOutsideLimits, kTooHeavy };

struct Plan {
  Verdict verdict = Verdict::kUnreachable;
  Arm3Solution solution;           // the way chosen, if the verdict is ok
  double torque_ratio = 0.0;       // the greatest torque, as a fraction of the limit of its motor, of the way chosen (or of the best way, if too heavy)
  double duration = 0.0;           // seconds, if ok
};

// The torque each of the three motors must supply to hold the arm still in this configuration, in newton metres, as magnitudes: for joint i, gravity times the sum over every mass beyond it
// of the mass times its horizontal distance from the joint. The arm is in a vertical plane with y up, and each angle is measured from the link before.
inline std::vector<double> holding_torques3(const ArmSpec& a, double q1, double q2, double q3) {
  const double a1 = q1, a2 = q1 + q2, a3 = q1 + q2 + q3;
  const double p1x = a.length[0] * std::cos(a1);
  const double p2x = p1x + a.length[1] * std::cos(a2);
  const double c1x = 0.5 * a.length[0] * std::cos(a1);
  const double c2x = p1x + 0.5 * a.length[1] * std::cos(a2);
  const double c3x = p2x + 0.5 * a.length[2] * std::cos(a3);
  const double tipx = p2x + a.length[2] * std::cos(a3);
  const double g = a.gravity;
  std::vector<double> t(3);
  t[0] = std::fabs(g * (a.mass[0] * c1x + a.mass[1] * c2x + a.mass[2] * c3x + a.payload * tipx));
  t[1] = std::fabs(g * (a.mass[1] * (c2x - p1x) + a.mass[2] * (c3x - p1x) + a.payload * (tipx - p1x)));
  t[2] = std::fabs(g * (a.mass[2] * (c3x - p2x) + a.payload * (tipx - p2x)));
  return t;
}

// Whether the arm can go from the configuration `from` to the pose `target`, and how. Both ways of reaching the pose are considered. The verdict is, in this order: unreachable if the wrist
// cannot be reached; outside the limits if neither way has every angle inside its limit; too heavy if neither of the ways inside the limits is one the motors can hold (the greatest torque
// of each no more than the limit of its motor); otherwise ok, with the way that takes the least time (elbow up if they tie), the torque ratio of that way, and the duration of the move from
// `from` with every joint arriving together. The angles of the way chosen are put inside their limits.
inline Plan plan_move(const ArmSpec& arm, const Arm3Solution& from, const Pose2& target) {
  Plan plan;
  const auto solutions = rc::kin::solve3(target, arm.length[0], arm.length[1], arm.length[2]);
  if (!solutions) return plan;

  const Arm3Solution candidates[2] = {solutions.value().elbow_up, solutions.value().elbow_down};
  bool any_legal = false;
  bool any_holdable = false;
  double best_duration = std::numeric_limits<double>::infinity();
  double best_ratio_among_legal = std::numeric_limits<double>::infinity();

  for (const Arm3Solution& c : candidates) {
    const double q[3] = {c.q1, c.q2, c.q3};
    bool legal = true;
    for (int i = 0; i < 3; ++i)
      if (!rc::kin::holds(arm.angle_limit[i], q[i])) legal = false;
    if (!legal) continue;
    any_legal = true;

    double folded[3];
    for (int i = 0; i < 3; ++i) folded[i] = rc::kin::at_same_direction(arm.angle_limit[i], q[i]);
    const std::vector<double> torque = holding_torques3(arm, folded[0], folded[1], folded[2]);
    double ratio = 0.0;
    for (int i = 0; i < 3; ++i) {
      const double r = arm.torque_limit[i] > 0.0 ? torque[i] / arm.torque_limit[i] : (torque[i] > 0.0 ? std::numeric_limits<double>::infinity() : 0.0);
      if (r > ratio) ratio = r;
    }
    if (ratio < best_ratio_among_legal) best_ratio_among_legal = ratio;
    if (ratio > 1.0) continue;
    any_holdable = true;

    const std::vector<double> distances = {folded[0] - from.q1, folded[1] - from.q2, folded[2] - from.q3};
    const rc::kin::Synchronised move = rc::kin::synchronise(distances, std::vector<double>(arm.vmax, arm.vmax + 3), std::vector<double>(arm.amax, arm.amax + 3));
    if (move.duration < best_duration) {
      best_duration = move.duration;
      plan.solution = Arm3Solution{folded[0], folded[1], folded[2]};
      plan.torque_ratio = ratio;
      plan.duration = move.duration;
    }
  }

  if (!any_legal) {
    plan.verdict = Verdict::kOutsideLimits;
    return plan;
  }
  if (!any_holdable) {
    plan.verdict = Verdict::kTooHeavy;
    plan.torque_ratio = best_ratio_among_legal;
    return plan;
  }
  plan.verdict = Verdict::kOk;
  return plan;
}

#endif  // LESSON_SOLUTION_HPP
