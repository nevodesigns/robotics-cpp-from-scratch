#include <rc/test/rc_test.hpp>

#include <cmath>
#include <iostream>

#include "solution.hpp"

namespace {
const double kPi = 3.14159265358979323846;

ArmSpec standard() {
  ArmSpec a;
  a.payload = 0.5;
  return a;
}

double max_ratio(const ArmSpec& a, const Arm3Solution& s) {
  const std::vector<double> t = holding_torques3(a, s.q1, s.q2, s.q3);
  double r = 0.0;
  for (int i = 0; i < 3; ++i) r = std::fmax(r, t[i] / a.torque_limit[i]);
  return r;
}
}  // namespace

RC_TEST("the torques that hold an arm still are gravity times the masses beyond each joint times their horizontal distance, and are nothing when it points straight up") {
  const ArmSpec a = standard();
  const std::vector<double> out = holding_torques3(a, 0.0, 0.0, 0.0);               // straight out along x
  std::cout << "  straight out, links 1, 1, 1, masses 2, 1.5, 1, payload 0.5: " << out[0] << ", " << out[1] << ", " << out[2] << " N m\n";
  RC_CHECK_NEAR(out[0], 9.81 * 7.25, 1e-9);        // 0.5*2 + 1.5*1.5 + 2.5*1 + 3*0.5 = 7.25
  RC_CHECK_NEAR(out[1], 9.81 * 3.25, 1e-9);        // 0.5*1.5 + 1.5*1 + 2*0.5
  RC_CHECK_NEAR(out[2], 9.81 * 1.0, 1e-9);         // 0.5*1 + 1*0.5

  const std::vector<double> up = holding_torques3(a, kPi / 2.0, 0.0, 0.0);          // straight up
  for (int i = 0; i < 3; ++i) RC_CHECK_NEAR(up[i], 0.0, 1e-12);
  const std::vector<double> down = holding_torques3(a, -kPi / 2.0, 0.0, 0.0);       // straight down: the same, nothing, and not a negative number
  for (int i = 0; i < 3; ++i) RC_CHECK_NEAR(down[i], 0.0, 1e-12);

  const std::vector<double> folded = holding_torques3(a, 0.0, kPi, 0.0);            // out, then back along itself
  RC_CHECK(folded[0] < out[0]);
  ArmSpec heavy = a;
  heavy.payload = 2.0;
  RC_CHECK(holding_torques3(heavy, 0.0, 0.0, 0.0)[0] > out[0]);                       // more payload, more torque
  RC_CHECK_NEAR(holding_torques3(heavy, 0.0, 0.0, 0.0)[2], 9.81 * (0.5 + 2.0), 1e-9);
}

RC_TEST("a pose that cannot be reached, one outside the joint limits, one too heavy and one that can be done are four different answers") {
  const Arm3Solution here{0.0, 0.0, 0.0};
  ArmSpec a = standard();

  const Plan ok = plan_move(a, here, Pose2{3.0, 0.0, 0.0});                          // straight out: legal and held
  const Plan far = plan_move(a, here, Pose2{2.9, 0.0, kPi});                         // the wrist would be at 3.9
  ArmSpec stiff = a;
  stiff.angle_limit[1] = rc::kin::Limit{-0.5, 0.5};                                  // an elbow that barely bends
  const Plan blocked = plan_move(stiff, here, Pose2{1.5, 0.5, 0.3});                 // needs an elbow of about plus or minus 2.55
  ArmSpec loaded = a;
  loaded.payload = 1.0;
  const Plan heavy = plan_move(loaded, here, Pose2{3.0, 0.0, 0.0});                  // the same straight out pose, with twice the payload

  std::cout << "  straight out with 0.5 kg: ok, " << ok.torque_ratio << " of the limit, " << ok.duration << " s; (2.9, 0) pointing back: " << (far.verdict == Verdict::kUnreachable ? "unreachable" : "?")
            << "; an elbow that barely bends: " << (blocked.verdict == Verdict::kOutsideLimits ? "outside the limits" : "?") << "; straight out with 1 kg: " << (heavy.verdict == Verdict::kTooHeavy ? "too heavy" : "?")
            << " at " << heavy.torque_ratio << " of the limit\n\n";
  RC_CHECK(ok.verdict == Verdict::kOk);
  RC_CHECK(far.verdict == Verdict::kUnreachable);
  RC_CHECK(blocked.verdict == Verdict::kOutsideLimits);
  RC_CHECK(heavy.verdict == Verdict::kTooHeavy);
  RC_CHECK_NEAR(ok.torque_ratio, 9.81 * 7.25 / 80.0, 1e-9);                           // the shoulder is the one that is nearest its limit: 0.889
  RC_CHECK(heavy.torque_ratio > 1.0);
  RC_CHECK_NEAR(heavy.torque_ratio, 9.81 * 8.75 / 80.0, 1e-9);
}

RC_TEST("the heaviest payload a pose can be held with is where a motor reaches its limit, and a little below is ok and a little above is too heavy") {
  const Arm3Solution here{0.0, 0.0, 0.0};
  const Pose2 target{3.0, 0.0, 0.0};
  double low = 0.0, high = 10.0;
  for (int i = 0; i < 60; ++i) {
    const double mid = 0.5 * (low + high);
    ArmSpec a = standard();
    a.payload = mid;
    if (plan_move(a, here, target).verdict == Verdict::kOk) low = mid;
    else high = mid;
  }
  ArmSpec at = standard();
  at.payload = low;
  const std::vector<double> t = holding_torques3(at, 0.0, 0.0, 0.0);
  std::cout << "  the heaviest payload for the pose straight out: " << low << " kg; the shoulder is then at " << t[0] << " of " << at.torque_limit[0] << " N m, the elbow " << t[1] << " of " << at.torque_limit[1]
            << ", the wrist " << t[2] << " of " << at.torque_limit[2] << "\n\n";
  RC_CHECK_NEAR(low, (80.0 / 9.81 - 5.75) / 3.0, 1e-6);                               // the shoulder: 9.81 (5.75 + 3 p) = 80
  RC_CHECK_NEAR(t[0], 80.0, 1e-4);                                                     // it is the shoulder that is at its limit
  RC_CHECK(t[1] < 40.0);
  RC_CHECK(t[2] < 15.0);
  ArmSpec below = standard(), above = standard();
  below.payload = low * 0.99;
  above.payload = low * 1.01;
  RC_CHECK(plan_move(below, here, target).verdict == Verdict::kOk);
  RC_CHECK(plan_move(above, here, target).verdict == Verdict::kTooHeavy);
}

RC_TEST("of the two ways to reach a pose the quicker is chosen, and when the motors can hold only one it is that one") {
  const ArmSpec a = standard();
  // A pose for which the two ways differ in what they ask of the motors. (For some poses they do not: the wrist motor holds the tool, whose heading is the same either way.)
  const Pose2 candidates[] = {{1.5, 0.5, 0.3}, {2.2, 0.3, 1.2}, {1.2, 1.0, 0.9}, {2.0, -0.5, 0.2}, {0.8, 1.6, 1.4}};
  Pose2 target = candidates[0];
  bool found = false;
  for (const Pose2& c : candidates) {
    const auto s = rc::kin::solve3(c, 1.0, 1.0, 1.0);
    if (!s) continue;
    const double ru = max_ratio(a, s.value().elbow_up), rd = max_ratio(a, s.value().elbow_down);
    if (std::fabs(ru - rd) > 0.02 * std::fmax(ru, rd)) {
      target = c;
      found = true;
      break;
    }
  }
  RC_CHECK(found);
  std::cout << "  the pose (" << target.x << ", " << target.y << ", " << target.phi << ") is one for which the two ways differ in what they ask of the motors\n";
  const auto both = rc::kin::solve3(target, 1.0, 1.0, 1.0);
  RC_CHECK(static_cast<bool>(both));
  if (!both) return;
  const Arm3Solution up = both.value().elbow_up, down = both.value().elbow_down;
  const Plan from_up = plan_move(a, Arm3Solution{up.q1 + 0.05, up.q2 + 0.05, up.q3}, target);
  const Plan from_down = plan_move(a, Arm3Solution{down.q1 + 0.05, down.q2 - 0.05, down.q3}, target);
  std::cout << "  starting near the elbow up way the plan takes the elbow " << (std::fabs(from_up.solution.q2 - up.q2) < 1e-9 ? "up" : "down") << " (" << from_up.duration << " s); near the elbow down way, "
            << (std::fabs(from_down.solution.q2 - down.q2) < 1e-9 ? "down" : "up") << " (" << from_down.duration << " s)\n";
  RC_CHECK(from_up.verdict == Verdict::kOk);
  RC_CHECK(from_down.verdict == Verdict::kOk);
  RC_CHECK_NEAR(from_up.solution.q2, up.q2, 1e-9);
  RC_CHECK_NEAR(from_down.solution.q2, down.q2, 1e-9);

  // Now the motors: tighten every limit to between the two ways, so that one can be held and the other cannot.
  const double r_up = max_ratio(a, up), r_down = max_ratio(a, down);
  std::cout << "  the two ways need " << r_up << " and " << r_down << " of the limits of the motors\n";
  RC_CHECK(std::fabs(r_up - r_down) > 0.01 * std::fmax(r_up, r_down));
  const double scale = 0.5 * (r_up + r_down);
  ArmSpec tight = a;
  for (int i = 0; i < 3; ++i) tight.torque_limit[i] *= scale;
  const bool up_is_easier = r_up < r_down;
  // Starting next to the harder way, the plan still takes the way that can be held.
  const Arm3Solution near_harder = up_is_easier ? Arm3Solution{down.q1, down.q2, down.q3} : Arm3Solution{up.q1, up.q2, up.q3};
  const Plan p = plan_move(tight, near_harder, target);
  const Arm3Solution easier = up_is_easier ? up : down;
  RC_CHECK(p.verdict == Verdict::kOk);
  RC_CHECK_NEAR(p.solution.q2, easier.q2, 1e-9);
  RC_CHECK(p.torque_ratio <= 1.0);
}

RC_TEST("an ok plan puts the tool on the pose within the limits, takes the time of the slowest joint, and a move that goes nowhere takes none") {
  const ArmSpec a = standard();
  const Arm3Solution start{0.2, -0.4, 0.3};
  const Pose2 target{1.8, 0.7, 0.5};
  const Plan p = plan_move(a, start, target);
  RC_CHECK(p.verdict == Verdict::kOk);
  const Pose2 got = rc::kin::forward3(1.0, 1.0, 1.0, p.solution.q1, p.solution.q2, p.solution.q3);
  RC_CHECK_NEAR(got.x, target.x, 1e-9);
  RC_CHECK_NEAR(got.y, target.y, 1e-9);
  RC_CHECK_NEAR(rc::kin::heading_error(target.phi, got.phi), 0.0, 1e-9);
  RC_CHECK(rc::kin::holds(a.angle_limit[1], p.solution.q2));
  RC_CHECK(rc::kin::holds(a.angle_limit[2], p.solution.q3));
  RC_CHECK(p.duration > 0.0);

  const std::vector<double> d = {p.solution.q1 - start.q1, p.solution.q2 - start.q2, p.solution.q3 - start.q3};
  const rc::kin::Synchronised move = rc::kin::synchronise(d, std::vector<double>(a.vmax, a.vmax + 3), std::vector<double>(a.amax, a.amax + 3));
  std::cout << "  from (0.2, -0.4, 0.3) to the pose (1.8, 0.7, 0.5): " << p.duration << " s, the same as the slowest of the three joints alone\n\n";
  RC_CHECK_NEAR(p.duration, move.duration, 1e-12);
  RC_CHECK(p.torque_ratio > 0.0 && p.torque_ratio <= 1.0);

  // From where the arm is already: nothing to do.
  const Plan stay = plan_move(a, Arm3Solution{p.solution.q1, p.solution.q2, p.solution.q3}, target);
  RC_CHECK(stay.verdict == Verdict::kOk);
  RC_CHECK_NEAR(stay.duration, 0.0, 1e-12);
}
