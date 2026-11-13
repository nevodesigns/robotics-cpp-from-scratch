#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <rc/drive/follow.hpp>
#include <rc/field/terrain.hpp>
#include <rc/vehicle/skid_steer.hpp>
#include <rc/vehicle/traction.hpp>
#include <vector>

using namespace rc::drive;
using namespace rc::vehicle;

struct SkidCommand {
  double v = 0.0;   // the speed asked for, m/s
  double w = 0.0;   // the turn rate asked for, rad/s
  double wl = 0.0;  // left wheel, rad/s
  double wr = 0.0;  // right wheel, rad/s
};

struct SkidFollowConfig {
  double lookahead = 0.6;
  double speed = 0.6;        // the speed wanted on a straight, m/s
  double min_speed = 0.15;   // the floor in a turn, m/s
  double margin = 0.7;       // the share of the grip a turn may use
  double chi = 1.0;          // the ICR coefficient the controller believes
  double slip = 0.0;         // the slip the controller believes
  bool limit_speed = false;  // cap the speed by the traction limit
  double dt = 0.02;
  int max_steps = 6000;
};

// Wheel speeds (rad/s) that make the body go at v and turn at w, given the effective track chi * B and the slip.
inline SkidCommand skid_wheel_speeds(const rc::vehicle::SkidGeometry& g, double chi, double slip, double v, double w) {
  SkidCommand c;
  c.v = v;
  c.w = w;
  const double sum = 2.0 * v / (g.wheel_radius * (1.0 - slip));
  const double diff = w * chi * g.track / g.wheel_radius;
  c.wl = 0.5 * (sum - diff);
  c.wr = 0.5 * (sum + diff);
  return c;
}

// The command for a pose on a path: pure pursuit's curvature, the speed capped by the surface, and the wheel speeds.
inline SkidCommand skid_follow_command(const rc::vehicle::SkidGeometry& g, const rc::field::Cell& cell, const SkidFollowConfig& cfg,
                                       const CarPose& pose, const std::vector<CarPose>& path, std::size_t idx) {
  std::size_t j = idx;
  while (j + 1 < path.size() && std::hypot(path[j].x - pose.x, path[j].y - pose.y) < cfg.lookahead) ++j;
  const double d = std::hypot(path[j].x - pose.x, path[j].y - pose.y);
  const double alpha = wrap_pi(std::atan2(path[j].y - pose.y, path[j].x - pose.x) - pose.theta);
  const double k = d > 1e-9 ? 2.0 * std::sin(alpha) / d : 0.0;
  double v = cfg.speed;
  if (cfg.limit_speed) {
    const double vmax = cfg.margin * rc::vehicle::max_speed_for_curvature(cell, pose.theta, k);
    v = std::max(cfg.min_speed, std::min(v, vmax));
  }
  return skid_wheel_speeds(g, cfg.chi, cfg.slip, v, v * k);
}

struct SkidRun {
  std::vector<CarPose> trace;
  double max_cte = 0.0;
  double final_cte = 0.0;
  int slid_steps = 0;   // steps in which the sideways acceleration reached what the surface grips
  double time = 0.0;    // seconds to the end of the path, or to the step limit
  bool reached = false;
};

// Drives a SkidSteer plant (the real chi and slip) from the origin, heading east, with the controller of `cfg`.
inline SkidRun skid_follow_run(const rc::vehicle::SkidGeometry& g, double chi_true, double slip_true, const rc::field::Cell& cell,
                               const SkidFollowConfig& cfg, const std::vector<CarPose>& path) {
  SkidRun run;
  rc::vehicle::SkidSteer plant(g, chi_true, slip_true);
  std::size_t idx = 0;
  for (int step = 0; step < cfg.max_steps; ++step) {
    const rc::vehicle::Pose2 q = plant.pose();
    CarPose pose;
    pose.x = q.x;
    pose.y = q.y;
    pose.theta = q.theta;
    idx = nearest_index(path, pose.x, pose.y, idx);
    const double cte = cross_track(path, pose.x, pose.y);
    run.max_cte = std::max(run.max_cte, std::fabs(cte));
    run.final_cte = std::fabs(cte);
    run.trace.push_back(pose);
    run.time = step * cfg.dt;
    if (idx + 1 >= path.size() && std::hypot(path.back().x - pose.x, path.back().y - pose.y) < 0.15) {
      run.reached = true;
      break;
    }
    const SkidCommand c = skid_follow_command(g, cell, cfg, pose, path, idx);
    // What the body will really do with those wheel speeds.
    const double v_true = 0.5 * g.wheel_radius * (c.wl + c.wr) * (1.0 - slip_true);
    const double w_true = g.wheel_radius * (c.wr - c.wl) / (chi_true * g.track);
    if (v_true > 1e-6) {
      const rc::vehicle::TractionBudget b = rc::vehicle::traction_budget(cell, pose.theta, v_true, w_true / v_true);
      if (b.slides) ++run.slid_steps;
    }
    plant.step(c.wl, c.wr, cfg.dt);
  }
  return run;
}


#endif
