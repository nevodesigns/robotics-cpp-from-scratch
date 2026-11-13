// rc/arm/moving_obstacle.hpp
//
// Getting out of the way of something that moves, from lesson 30-08.
//
// Track is where a thing was last seen and how fast it is going, and track_update works a velocity out from a new sighting by differencing and smoothing it with the old one. track_predict says where it will be a time later, by the straight line. time_to_collision looks along an arm's planned trajectory (lesson 30-06) from a time, at steps, for the first moment at which the arm's capsules come within a margin of the predicted obstacle, and gives the time to it, or a negative number if there is none within the horizon. run_with_moving_obstacle is the whole behaviour in simulation: the arm follows the trajectory; at each control tick the track is updated and the plan is looked along; if a meeting is predicted within the horizon the arm stops where it is (at once, in this simulation) and either waits and looks again at the next tick, or, if it is told to plan, makes a new plan from where it stands (RRT-Connect, shortcut, timed) against the obstacle made a little larger and goes on; the monitor goes on looking along the new plan too. It reports whether it got to the goal, the time, the number of plans, the ticks it waited, and the smallest clearance between the arm and the obstacle's true place over the run.

#ifndef RC_RC_ARM_MOVING_OBSTACLE_HPP
#define RC_RC_ARM_MOVING_OBSTACLE_HPP

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <rc/arm/rrt.hpp>
#include <rc/arm/shortcut.hpp>
#include <rc/arm/time_param.hpp>
#include <vector>

namespace rc {
namespace arm {

struct Track {
  double p[3] = {0.0, 0.0, 0.0};
  double v[3] = {0.0, 0.0, 0.0};
  double t = 0.0;
  bool seen = false;
};

// A new sighting: the velocity is the difference since the last one, smoothed with the old (alpha is the share of the new).
inline void track_update(Track& tr, const double p[3], double t, double alpha) {
  if (tr.seen && t > tr.t) {
    for (int k = 0; k < 3; ++k) {
      const double v = (p[k] - tr.p[k]) / (t - tr.t);
      tr.v[k] = alpha * v + (1.0 - alpha) * tr.v[k];
    }
  }
  for (int k = 0; k < 3; ++k) tr.p[k] = p[k];
  tr.t = t;
  tr.seen = true;
}

// Where the thing will be a time dt after its last sighting, by the straight line.
inline void track_predict(const Track& tr, double dt, double out[3]) {
  for (int k = 0; k < 3; ++k) out[k] = tr.p[k] + tr.v[k] * dt;
}

// Looks along a trajectory from a time (measured in the trajectory) for the first moment the arm comes within `margin` of the predicted obstacle; the time from now to it, or -1 if there is none within the horizon.
inline double time_to_collision(const ArmTable& t, const std::vector<double>& radii, double tool_length, const std::vector<TrajSeg>& traj, double traj_time, const Track& tr, double now, double obstacle_radius, double margin, double horizon, double step) {
  const double total = trajectory_duration(traj);
  std::vector<double> q, qd, qdd, qddd;
  for (double dt = 0.0; dt <= horizon + 1e-12; dt += step) {
    const double at = traj_time + dt;
    sample_trajectory(traj, at < total ? at : total, q, qd, qdd, qddd);
    Sphere s;
    track_predict(tr, (now - tr.t) + dt, s.c);
    s.r = obstacle_radius;
    const std::vector<Capsule> caps = arm_capsules(t, q, radii, tool_length);
    Scene sc;
    sc.spheres.push_back(s);
    if (scene_distance(caps, sc, false).d < margin) return dt;
  }
  return -1.0;
}

struct ReplanConfig {
  double dt = 0.05;          // the control tick, seconds
  double horizon = 1.0;      // how far ahead the plan is looked along, seconds
  double margin = 0.03;      // metres of room to keep from the obstacle
  double plan_extra = 0.10;  // metres larger than it is that the obstacle is taken to be when planning round it
  bool monitor = true;       // look along the plan
  bool replan = true;        // on a predicted meeting, plan again (otherwise wait where it stands and look again next tick)
  double max_time = 12.0;
};

struct ReplanRun {
  bool reached = false;
  double time = 0.0;
  int plans = 0;
  int waits = 0;             // ticks spent standing still
  double min_clearance = 1e9;
};

// The arm follows a trajectory from `start` to `goal` while a sphere moves in a straight line at a constant speed; see the header. The stop when a meeting is predicted is instant in this simulation.
inline ReplanRun run_with_moving_obstacle(const ArmTable& table, const std::vector<double>& radii, double tool_length, const JointLimits& lim, const std::vector<double>& start, const std::vector<double>& goal,
                                          const double obs_start[3], const double obs_vel[3], double obs_radius, const ReplanConfig& cfg, unsigned long long seed) {
  ReplanRun out;
  std::vector<std::vector<double> > path(2);
  path[0] = start;
  path[1] = goal;
  std::vector<TrajSeg> traj = plan_trajectory(path, lim);
  double traj_time = 0.0;
  std::vector<double> q = start, qd, qdd, qddd;
  Track track;
  const std::vector<double> bounds = sweep_bounds(table, radii, tool_length);
  for (double now = 0.0; now < cfg.max_time; now += cfg.dt) {
    double obs[3];
    for (int k = 0; k < 3; ++k) obs[k] = obs_start[k] + obs_vel[k] * now;
    track_update(track, obs, now, 0.7);
    // The true clearance between the arm and the obstacle.
    Scene truth;
    Sphere s;
    for (int k = 0; k < 3; ++k) s.c[k] = obs[k];
    s.r = obs_radius;
    truth.spheres.push_back(s);
    out.min_clearance = std::fmin(out.min_clearance, scene_distance(arm_capsules(table, q, radii, tool_length), truth, false).d);
    if (traj_time >= trajectory_duration(traj) && joint_distance(q, goal) < 1e-3) {
      out.reached = true;
      out.time = now;
      return out;
    }
    bool hold = false;
    const double speed2 = track.v[0] * track.v[0] + track.v[1] * track.v[1] + track.v[2] * track.v[2];
    if (cfg.monitor && speed2 > 1e-6 && time_to_collision(table, radii, tool_length, traj, traj_time, track, now, obs_radius, cfg.margin, cfg.horizon, cfg.dt) >= 0.0) {
      hold = true;  // a meeting is predicted: stop
      if (cfg.replan) {
        Scene plan_scene;
        Sphere ps = s;
        ps.r = obs_radius + cfg.plan_extra;
        plan_scene.spheres.push_back(ps);
        PlanProblem p;
        p.check.table = &table;
        p.check.radii = &radii;
        p.check.tool_length = tool_length;
        p.check.scene = &plan_scene;
        p.check.self = false;
        p.bounds = bounds;
        p.seed = seed + static_cast<unsigned long long>(out.plans);
        p.max_iter = 1500;
        const PlanResult r = rrt_connect(p, q, goal);  // nothing, at once, if the arm is already inside what it must avoid
        ++out.plans;
        if (r.found) {
          path = shortcut(p, r.path, 100, seed + 7, p.margin);
          traj = plan_trajectory(path, lim);
          traj_time = 0.0;
          hold = false;
        }
      }
      if (hold) ++out.waits;
    }
    if (!hold) {
      traj_time += cfg.dt;
      const double tot = trajectory_duration(traj);
      sample_trajectory(traj, traj_time < tot ? traj_time : tot, q, qd, qdd, qddd);
    }
  }
  out.time = cfg.max_time;
  return out;
}


}  // namespace arm
}  // namespace rc

#endif
