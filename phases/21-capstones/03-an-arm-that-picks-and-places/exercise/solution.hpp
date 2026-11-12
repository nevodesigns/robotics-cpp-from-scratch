#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <rc/control/limits.hpp>
#include <rc/kin/limits.hpp>
#include <rc/kin/path.hpp>
#include <rc/kin/planar_arm.hpp>
#include <rc/kin/timing.hpp>
#include <rc/safety/limiter.hpp>
#include <rc/safety/supervisor.hpp>
#include <vector>

// A two link arm in a plane, with its joint limits and the speed and acceleration each joint may have (radians a second, and a second squared).
struct ArmCell {
  double l1 = 0.4;
  double l2 = 0.3;
  rc::kin::ArmLimits limits;
  double vmax = 2.0;
  double amax = 6.0;
};

inline ArmCell default_arm_cell() {
  ArmCell c;
  c.limits.shoulder = {-0.6, 2.4};
  c.limits.elbow = {-2.4, 2.4};
  return c;
}

struct ArmXY {
  double x = 0.0;
  double y = 0.0;
};

// Where the tip is for a pair of joint angles: the forward kinematics, x = l1 cos q1 + l2 cos (q1 + q2), and the same with sines for y.
inline ArmXY arm_tip(const ArmCell& c, double q1, double q2) {
  // TODO: x = l1 cos(q1) + l2 cos(q1 + q2), y = l1 sin(q1) + l2 sin(q1 + q2)
  (void)c;
  (void)q1;
  (void)q2;
  return ArmXY();
}

// The joint angles to put the tip at (x, y): of the two elbow branches, the elbow down one if it is inside the joint limits, otherwise the elbow up one if that is, otherwise there is none and the answer is false. A point that cannot be reached at all is false.
inline bool arm_pose_for(const ArmCell& c, double x, double y, rc::kin::JointPose& pose) {
  // TODO: rc::kin::legal_solutions(x, y, l1, l2, limits): not reachable (no value) is false; the elbow down pose if down_holds, else the elbow up pose if up_holds (copy q1 and q2 into `pose`), else false
  (void)c;
  (void)x;
  (void)y;
  (void)pose;
  return false;
}

// A cycle of the arm: a closed loop through points, each a move of both joints in step (all joints taking as long as the slowest, lesson 13-14) with a trapezoid of speed.
struct ArmPlan {
  bool ok = false;
  int bad_point = -1;                                  // the first point with no legal pose, when not ok
  std::vector<rc::kin::JointPose> poses;
  std::vector<rc::kin::Synchronised> legs;             // leg i goes from point i to point i + 1, and the last back to the first
  double duration = 0.0;
};

// Plan the loop: a pose for each point (arm_pose_for; the first that has none gives ok false and bad_point), and for each leg the synchronised moves of the two joints by the cell's vmax and amax, the joint distances taken as the plain difference of the angles.
inline ArmPlan plan_cycle(const ArmCell& c, const std::vector<ArmXY>& points) {
  // TODO: a pose for each point by arm_pose_for (the first with none: bad_point = its index and return, ok false); then for each leg i (point i to point i + 1, the last back to the first) rc::kin::synchronise of the two joint distances (b.q - a.q) with {vmax, vmax} and {amax, amax}, added to duration; ok if there are poses
  (void)c;
  (void)points;
  return ArmPlan();
}

// The joint angles of leg `leg` at time `tau` into it.
inline rc::kin::JointPose pose_on_leg(const ArmPlan& plan, std::size_t leg, double tau) {
  const rc::kin::JointPose& a = plan.poses[leg];
  rc::kin::JointPose p = a;
  p.q1 = a.q1 + rc::kin::position_at(plan.legs[leg].joints[0], tau);
  p.q2 = a.q2 + rc::kin::position_at(plan.legs[leg].joints[1], tau);
  return p;
}

// Somebody who walks up to the cell: appears at (x, y) at `from`, walks to (x_end, y_end) by `arrive`, stands there and is gone at `to`.
struct ArmHuman {
  double x = 0.0;
  double y = 0.0;
  double x_end = 0.0;
  double y_end = 0.0;
  double from = 0.0;
  double arrive = 0.0;
  double to = 0.0;
  bool present = false;
};

inline bool human_at(const ArmHuman& h, double t, double& px, double& py) {
  // TODO: false if not present or t is before `from` or from `to` on; otherwise f = 1 once t has reached `arrive` (or arrive is not after from), else (t - from) / (arrive - from); px = x + (x_end - x) f, the same for y; true
  (void)h;
  (void)t;
  (void)px;
  (void)py;
  return false;
}

struct ArmRunConfig {
  double tip_limit = 0.8;           // the greatest speed of the tip near people, metres a second (the supervisor's top speed)
  bool use_monitor = true;          // slow the arm by the distance to the person, with the limiter of 19-05
  bool use_supervisor = true;
  double sensor_dead_at = -1.0;     // the person sensor reads NaN from this time
  double estop_at = -1.0;           // the e-stop loop opens at this time
  double contact_distance = 0.12;   // the tip is in contact with the person when closer than this
};

struct ArmRunResult {
  int cycles = 0;
  double first_cycle_time = 0.0;    // when the first cycle was finished, 0 if it never was
  double max_tip_speed = 0.0;
  double min_distance = 1e9;        // the closest the tip came to the person
  int contacts = 0;                 // ticks of 0.01 s with the tip closer than contact_distance
  bool stopped = false;
  unsigned causes = 0;
  double stop_tip_travel = 0.0;     // how far the tip went between the supervisor's stop and standing still
  double time = 0.0;
};

// Run the cell for `seconds` at 100 ticks a second, starting from the first point at rest, round and round the loop. Every tick:
//  - the distance from the tip to the person (infinity if there is none, NaN if the sensor is dead);
//  - the speed the limiter allows for that distance (a deceleration of 2 m/s^2, a latency of 0.1 s, a margin of 0.15 m, top speed the tip limit, nothing returned counting as 2 m), given to the supervisor as its range cap, with the e-stop and the sensor (faulted after 3 ticks of NaN) as inputs;
//  - the speed the plan would give the tip if it ran at full speed over the next tick, and so the fraction k of that which the cap allows (0 if stopped, and at most 1); k moves towards it by at most 6 a second down and 3 a second up;
//  - the plan's own time advances by k times the tick. When a leg is finished the next begins, and at the end of the loop a cycle is counted.
// After a stop the run ends when the tip is still.
inline ArmRunResult run_cell(const ArmCell& cell, const ArmPlan& plan, const ArmHuman& human, const ArmRunConfig& cfg, double seconds) {
  ArmRunResult r;
  if (!plan.ok) return r;
  const double dt = 0.01;
  rc::safety::LimiterConfig lc;
  lc.decel = 2.0;
  lc.latency = 0.1;
  lc.margin = 0.15;
  lc.top_speed = cfg.tip_limit;
  lc.no_return_range = 2.0;
  lc.max_valid = 4.0;
  rc::safety::SupervisorConfig sc;
  sc.top_speed = cfg.tip_limit;
  sc.slow_speed = 0.5 * cfg.tip_limit;
  rc::safety::Supervisor sup(sc);
  rc::safety::Inputs healthy;
  healthy.estop_closed = true;
  healthy.heartbeat_fresh = true;
  healthy.fence = rc::safety::FenceState::inside;
  healthy.tilt = rc::safety::TiltState::ok;
  healthy.sensors = rc::safety::CheckState::ok;
  healthy.power.state = rc::safety::PowerState::ok;
  healthy.power.factor = 1.0;
  healthy.range_cap = cfg.tip_limit + 1.0;
  sup.reset(healthy);
  std::size_t leg = 0;
  double tau = 0.0;
  double k = 0.0;
  int dead_ticks = 0;
  bool stopping = false;
  const int steps = static_cast<int>(seconds / dt);
  for (int i = 0; i < steps; ++i) {
    const double t = i * dt;
    const rc::kin::JointPose q = pose_on_leg(plan, leg, tau);
    const ArmXY tip = arm_tip(cell, q.q1, q.q2);
    double hx = 0.0;
    double hy = 0.0;
    double d = INFINITY;
    if (human_at(human, t, hx, hy)) d = std::hypot(tip.x - hx, tip.y - hy);
    const bool sensor_dead = cfg.sensor_dead_at >= 0.0 && t >= cfg.sensor_dead_at;
    if (sensor_dead) d = std::nan("");
    if (std::isfinite(d)) {
      r.min_distance = std::min(r.min_distance, d);
      if (d < cfg.contact_distance) ++r.contacts;
    }
    rc::safety::Reading reading;
    reading.range = d;
    rc::safety::Reading behind;
    behind.range = 5.0;
    const double allowed = std::fabs(rc::safety::limit_speed(cfg.tip_limit, reading, behind, lc));
    double cap = cfg.use_monitor ? allowed : cfg.tip_limit;
    bool stopped = false;
    if (cfg.use_supervisor) {
      dead_ticks = std::isnan(d) ? dead_ticks + 1 : 0;
      rc::safety::Inputs in = healthy;
      in.estop_closed = !(cfg.estop_at >= 0.0 && t >= cfg.estop_at);
      in.sensors = dead_ticks >= 3 ? rc::safety::CheckState::faulted : (dead_ticks > 0 ? rc::safety::CheckState::suspect : rc::safety::CheckState::ok);
      in.range_cap = cap;
      const rc::safety::Decision dec = sup.step(in);
      cap = dec.speed_cap;
      if (dec.mode == rc::safety::Mode::stopped) {
        stopped = true;
        r.causes |= dec.stop_causes;
        r.stopped = true;
      }
    }
    // the speed the tip would have at full speed over the next tick
    const double leg_duration = plan.legs[leg].duration;
    const rc::kin::JointPose q2 = pose_on_leg(plan, leg, std::min(tau + dt, leg_duration));
    const ArmXY tip2 = arm_tip(cell, q2.q1, q2.q2);
    const double v_nominal = std::hypot(tip2.x - tip.x, tip2.y - tip.y) / dt;
    double target = 1.0;
    if (stopped) target = 0.0;
    else if (v_nominal > 1e-9) target = std::max(0.0, std::min(1.0, cap / v_nominal));
    const double rate = target < k ? 6.0 : 3.0;
    k = rc::control::rate_limit(k, target, rate * dt);
    const double tip_speed = k * v_nominal;
    r.max_tip_speed = std::max(r.max_tip_speed, tip_speed);
    if (stopped && !stopping) stopping = true;
    if (stopping) r.stop_tip_travel += tip_speed * dt;
    tau += k * dt;
    if (tau >= leg_duration) {
      tau = 0.0;
      ++leg;
      if (leg >= plan.legs.size()) {
        leg = 0;
        ++r.cycles;
        if (r.cycles == 1) r.first_cycle_time = t + dt;
      }
    }
    r.time = t + dt;
    if (stopping && k <= 1e-6) break;
  }
  return r;
}

#endif  // LESSON_SOLUTION_HPP
