#include <rc/test/rc_test.hpp>

#include <cmath>
#include <iomanip>
#include <iostream>

#include "solution.hpp"

namespace {

const std::vector<ArmXY> kLoop = {{0.35, 0.35}, {0.5, 0.25}, {0.5, 0.15}, {0.5, 0.25}, {-0.05, 0.5}, {-0.05, 0.4}, {-0.05, 0.5}};

// Somebody walks up to the pick point and stands 8 cm from it until 9 s.
ArmHuman at_the_pick() {
  ArmHuman h;
  h.present = true;
  h.x = 1.3;
  h.y = 0.15;
  h.x_end = 0.58;
  h.y_end = 0.15;
  h.from = 1.0;
  h.arrive = 2.5;
  h.to = 9.0;
  return h;
}

ArmRunResult run(const ArmRunConfig& c, const ArmHuman& h = ArmHuman(), double seconds = 30.0) {
  const ArmCell cell = default_arm_cell();
  return run_cell(cell, plan_cycle(cell, kLoop), h, c, seconds);
}

}  // namespace

RC_TEST("the tip is where the angles say, a point gets the elbow branch that fits the limits, and a point out of reach or out of the limits gets none") {
  const ArmCell c = default_arm_cell();
  for (const ArmXY& p : kLoop) {
    rc::kin::JointPose q;
    RC_CHECK(arm_pose_for(c, p.x, p.y, q));
    const ArmXY back = arm_tip(c, q.q1, q.q2);
    RC_CHECK_NEAR(back.x, p.x, 1e-9);
    RC_CHECK_NEAR(back.y, p.y, 1e-9);
    RC_CHECK(rc::kin::holds(c.limits, rc::kin::ArmSolution{q.q1, q.q2}));
  }
  rc::kin::JointPose q;
  RC_CHECK(!arm_pose_for(c, 1.0, 0.5, q));                          // 1.12 m away: the arm is 0.7 m long
  RC_CHECK(!arm_pose_for(c, 0.02, 0.02, q));                        // closer than the 0.1 m the two links allow
  ArmCell tight = c;
  tight.limits.elbow = {-2.4, 0.5};                                  // an elbow that cannot bend far one way
  rc::kin::JointPose up;
  RC_CHECK(arm_pose_for(tight, 0.5, 0.15, up));
  RC_CHECK(up.q2 < 0.0);                                             // so it is the other branch
  ArmCell none = c;
  none.limits.elbow = {-0.5, 0.5};
  RC_CHECK(!arm_pose_for(none, 0.5, 0.15, q));
  std::cout << "  the five points of the loop are reached with the angles that put the tip there; (1.0, 0.5) is out of reach; with the elbow held to [-2.4, 0.5] the point (0.5, 0.15) takes the elbow up branch (q2 " << std::fixed << std::setprecision(2) << up.q2 << ")\n\n";
}

RC_TEST("a loop through the points has a pose for each and a timed move for each leg, and the first point that cannot be reached is named") {
  const ArmCell c = default_arm_cell();
  const ArmPlan plan = plan_cycle(c, kLoop);
  RC_CHECK(plan.ok);
  RC_CHECK(plan.bad_point == -1);
  RC_CHECK(plan.poses.size() == 7 && plan.legs.size() == 7);
  if (plan.legs.size() != 7) return;
  double sum = 0.0;
  for (const rc::kin::Synchronised& l : plan.legs) sum += l.duration;
  RC_CHECK_NEAR(sum, plan.duration, 1e-12);
  std::cout << "  seven points, seven legs, " << std::fixed << std::setprecision(2) << plan.duration << " s for the loop at the joints' own limits (" << c.vmax << " rad/s, " << c.amax << " rad/s^2)\n";
  std::vector<ArmXY> bad = kLoop;
  bad[3] = {1.2, 0.0};
  const ArmPlan broken = plan_cycle(c, bad);
  RC_CHECK(!broken.ok && broken.bad_point == 3);
  RC_CHECK(!plan_cycle(c, {}).ok);
  std::cout << "  a loop with the point 3 out of reach: not ok, bad point " << broken.bad_point << "\n\n";
}

RC_TEST("a limit on the speed of the tip costs a little at 0.8 m/s and a lot at 0.25, and the limit is held with a margin that the ramp makes") {
  const double limits[] = {5.0, 0.8, 0.25};
  ArmRunResult r[3];
  std::cout << "  the loop with nobody there, by the speed limit of the tip:\n  limit      first cycle   fastest tip   cycles in 60 s\n";
  for (int i = 0; i < 3; ++i) {
    ArmRunConfig c;
    c.tip_limit = limits[i];
    r[i] = run(c, ArmHuman(), 60.0);
    std::cout << "  " << std::fixed << std::setprecision(2) << std::setw(5) << limits[i] << " m/s  " << std::setw(8) << r[i].first_cycle_time << " s   " << std::setw(8) << r[i].max_tip_speed << " m/s   " << std::setw(3) << r[i].cycles << "\n";
  }
  std::cout << "\n";
  RC_CHECK(r[0].first_cycle_time > 4.0 && r[0].first_cycle_time < 4.2);
  RC_CHECK(r[1].first_cycle_time > r[0].first_cycle_time);
  RC_CHECK(r[1].first_cycle_time < 1.1 * r[0].first_cycle_time);
  RC_CHECK(r[2].first_cycle_time > 1.7 * r[0].first_cycle_time);
  RC_CHECK(r[0].max_tip_speed > 1.0);
  RC_CHECK(r[1].max_tip_speed <= 0.8 + 1e-6);
  RC_CHECK(r[2].max_tip_speed > 0.25 && r[2].max_tip_speed < 0.35);       // a limit held by a ramp is a target and not a wall
}

RC_TEST("somebody stands at the pick point, and the arm holds short of them where an arm that does not look carries on to 8 cm and for 66 ticks is in contact") {
  const ArmHuman h = at_the_pick();
  ArmRunConfig on;
  ArmRunConfig off;
  off.use_monitor = false;
  off.use_supervisor = false;
  const ArmRunResult a = run(on, h);
  const ArmRunResult b = run(off, h);
  const ArmRunResult clear = run(on);
  std::cout << "  somebody 8 cm from the pick point from 2.5 s to 9 s:\n";
  std::cout << "  monitored:   closest " << std::fixed << std::setprecision(3) << a.min_distance << " m, " << a.contacts << " ticks in contact, " << a.cycles << " cycles in 30 s (" << clear.cycles << " with nobody there)\n";
  std::cout << "  not looking: closest " << b.min_distance << " m, " << b.contacts << " ticks in contact, " << b.cycles << " cycles\n\n";
  RC_CHECK(a.contacts == 0);
  RC_CHECK(a.min_distance >= 0.14);
  RC_CHECK(b.contacts > 30);
  RC_CHECK(b.min_distance < 0.1);
  RC_CHECK(a.cycles <= clear.cycles);
  RC_CHECK(a.cycles >= 1);
}

RC_TEST("a person sensor that dies is a stop of the arm with its cause, within a centimetre or two of the tip") {
  ArmRunConfig c;
  c.sensor_dead_at = 3.0;
  const ArmRunResult r = run(c, at_the_pick());
  std::cout << "  the person sensor dead from 3 s: stopped " << (r.stopped ? "yes" : "no") << ", cause " << rc::safety::causes_text(r.causes) << ", the tip went " << std::fixed << std::setprecision(3) << r.stop_tip_travel << " m further, at " << std::setprecision(2) << r.time << " s\n\n";
  RC_CHECK(r.stopped);
  RC_CHECK(r.causes == rc::safety::cause_sensors);
  RC_CHECK(r.stop_tip_travel < 0.02);
  RC_CHECK(r.time > 3.0 && r.time < 3.3);
}

RC_TEST("an open e-stop loop stops the arm in a distance that grows with the speed it had") {
  ArmRunConfig slow;
  slow.estop_at = 2.0;
  ArmRunConfig fast = slow;
  fast.tip_limit = 5.0;
  const ArmRunResult a = run(slow);
  const ArmRunResult b = run(fast);
  std::cout << "  the e-stop opened at 2 s: at the 0.8 m/s limit the tip went " << std::fixed << std::setprecision(3) << a.stop_tip_travel << " m further; with the limit at 5 m/s (the arm's own speed, up to " << std::setprecision(2) << b.max_tip_speed << " m/s) " << std::setprecision(3) << b.stop_tip_travel << " m\n\n";
  RC_CHECK(a.stopped && b.stopped);
  RC_CHECK(a.causes == rc::safety::cause_estop && b.causes == rc::safety::cause_estop);
  RC_CHECK(a.stop_tip_travel < 0.06);
  RC_CHECK(b.stop_tip_travel > a.stop_tip_travel);
}
