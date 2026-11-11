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
  // TODO: the absolute angles a1 = q1, a2 = q1 + q2, a3 = q1 + q2 + q3. The x of the end of link 1: p1x = l1 cos(a1); of link 2: p2x = p1x + l2 cos(a2); the tool: tipx = p2x + l3 cos(a3). The
  // centres of the links (half way along): c1x = 0.5 l1 cos(a1), c2x = p1x + 0.5 l2 cos(a2), c3x = p2x + 0.5 l3 cos(a3). Torque 1 = |g (m1 c1x + m2 c2x + m3 c3x + payload tipx)|;
  // torque 2 = |g (m2 (c2x - p1x) + m3 (c3x - p1x) + payload (tipx - p1x))|; torque 3 = |g (m3 (c3x - p2x) + payload (tipx - p2x))|. Return the three.
  (void)a;
  (void)q1;
  (void)q2;
  (void)q3;
  return std::vector<double>(3, -1.0);
}

// Whether the arm can go from the configuration `from` to the pose `target`, and how. Both ways of reaching the pose are considered. The verdict is, in this order: unreachable if the wrist
// cannot be reached; outside the limits if neither way has every angle inside its limit; too heavy if neither of the ways inside the limits is one the motors can hold (the greatest torque
// of each no more than the limit of its motor); otherwise ok, with the way that takes the least time (elbow up if they tie), the torque ratio of that way, and the duration of the move from
// `from` with every joint arriving together. The angles of the way chosen are put inside their limits.
inline Plan plan_move(const ArmSpec& arm, const Arm3Solution& from, const Pose2& target) {
  // TODO: solutions = rc::kin::solve3(target, lengths...). If it is an error: the plan stays as it is (verdict Unreachable). For each of the two solutions: it is legal if rc::kin::holds(arm.angle_limit[i], q_i)
  // for all three; if none is legal the verdict is OutsideLimits. For each legal one, fold the angles with rc::kin::at_same_direction, work out holding_torques3, and the ratio is the greatest of
  // torque / torque_limit. Track the smallest ratio among the legal ones. A ratio above 1 is not holdable. For each holdable one, the duration of the move from `from` is rc::kin::synchronise(
  // the three differences folded - from, vmax, amax).duration; keep the way with the shortest (the first, elbow up, if they tie), with its angles, ratio and duration. If there are legal ones and
  // none is holdable, the verdict is TooHeavy and torque_ratio is the smallest ratio. Otherwise Ok.
  (void)arm;
  (void)from;
  (void)target;
  Plan plan;
  plan.verdict = Verdict::kOutsideLimits;
  plan.duration = -1.0;
  return plan;
}

#endif  // LESSON_SOLUTION_HPP
