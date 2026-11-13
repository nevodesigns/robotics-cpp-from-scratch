#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <rc/drive/follow.hpp>
#include <vector>

using namespace rc::drive;

struct MotionModel {
  bool bicycle = false;       // false: a unicycle that can turn on the spot
  double wheelbase = 0.33;    // metres, for a bicycle
  double max_steer = 0.4189;  // radians, for a bicycle
  double max_v = 1.0;         // metres per second
  double max_w = 2.0;         // radians per second
};

inline double model_max_curvature(const MotionModel& m) { return m.bicycle ? std::tan(m.max_steer) / m.wheelbase : 1e9; }

// One step of a motion model: speed v, yaw rate w, for dt seconds. The controls are clamped to what the model can do.
inline CarPose step_model(const MotionModel& m, const CarPose& p, double v, double w, double dt) {
  v = v < 0.0 ? 0.0 : (v > m.max_v ? m.max_v : v);
  w = w > m.max_w ? m.max_w : (w < -m.max_w ? -m.max_w : w);
  if (m.bicycle) {
    const double wmax = v * model_max_curvature(m);
    w = w > wmax ? wmax : (w < -wmax ? -wmax : w);
  }
  CarPose q = p;
  const double mid = p.theta + 0.5 * w * dt;
  q.x += v * dt * std::cos(mid);
  q.y += v * dt * std::sin(mid);
  q.theta = wrap_pi(p.theta + w * dt);
  return q;
}

struct Lcg {
  std::uint64_t s = 1;
  double uniform() {
    s = s * 6364136223846793005ULL + 1442695040888963407ULL;
    return static_cast<double>((s >> 33) & 0xFFFFFFULL) / 16777216.0;
  }
  double normal() {
    double a = 0.0;
    for (int i = 0; i < 12; ++i) a += uniform();
    return a - 6.0;
  }
};

struct MppiConfig {
  int samples = 100;
  int horizon = 16;
  double dt = 0.1;
  double lambda = 2.0;
  double sigma_v = 0.3;
  double sigma_w = 0.8;
  double path_weight = 4.0;       // per squared metre from the path, at every step
  double goal_weight = 2.0;       // per squared metre from the point ahead, at the end
  double cell_weight = 2.0;       // per step, times the cell cost over 255
  double collision = 200.0;       // per step in a blocked cell
  int ahead_points = 15;          // how many path points ahead the end of a rollout aims
  int window_points = 25;         // how many path points are searched for the distance to the path
  int replan_every = 1;           // control steps between plans; in between, the last plan is played out
};

struct MppiState {
  std::vector<double> v;
  std::vector<double> w;
  Lcg rng;
};

struct Command {
  double v = 0.0;
  double w = 0.0;
};

inline bool blocked_at(const CostGrid* grid, double x, double y) {
  return grid != nullptr && grid->at(grid->cell_i(x), grid->cell_j(y)) >= rc::perceive::kCostInscribed;
}

// One control: the first of the improved nominal sequence. `idx` is the pose's place on the path.
inline Command mppi_control(const MotionModel& model, const MppiConfig& cfg, const CarPose& pose, const std::vector<CarPose>& path,
                            std::size_t idx, const CostGrid* grid, MppiState& st) {
  const int H = cfg.horizon, K = cfg.samples;
  if (static_cast<int>(st.v.size()) != H) {
    st.v.assign(static_cast<std::size_t>(H), 0.7 * model.max_v);
    st.w.assign(static_cast<std::size_t>(H), 0.0);
  }
  const std::size_t hi = std::min(path.size(), idx + static_cast<std::size_t>(cfg.window_points));
  const CarPose& target = path[std::min(path.size() - 1, idx + static_cast<std::size_t>(cfg.ahead_points))];
  std::vector<double> cost(static_cast<std::size_t>(K), 0.0);
  std::vector<double> ev(static_cast<std::size_t>(K * H)), ew(static_cast<std::size_t>(K * H));
  for (int i = 0; i < K; ++i) {
    CarPose p = pose;
    double c = 0.0;
    for (int k = 0; k < H; ++k) {
      const std::size_t at = static_cast<std::size_t>(i * H + k);
      const double v = std::min(model.max_v, std::max(0.0, st.v[static_cast<std::size_t>(k)] + cfg.sigma_v * st.rng.normal()));
      const double w = std::min(model.max_w, std::max(-model.max_w, st.w[static_cast<std::size_t>(k)] + cfg.sigma_w * st.rng.normal()));
      ev[at] = v - st.v[static_cast<std::size_t>(k)];
      ew[at] = w - st.w[static_cast<std::size_t>(k)];
      p = step_model(model, p, v, w, cfg.dt);
      double nearest = 1e18;
      for (std::size_t j = idx; j < hi; ++j) nearest = std::min(nearest, std::hypot(path[j].x - p.x, path[j].y - p.y));
      c += cfg.path_weight * nearest * nearest;
      if (blocked_at(grid, p.x, p.y)) {
        c += cfg.collision;
      } else if (grid != nullptr) {
        c += cfg.cell_weight * grid->at(grid->cell_i(p.x), grid->cell_j(p.y)) / 255.0;
      }
    }
    const double dg = std::hypot(target.x - p.x, target.y - p.y);
    cost[static_cast<std::size_t>(i)] = c + cfg.goal_weight * dg * dg;
  }
  (void)ev;  // TODO: weight_i = exp(-(cost_i - least cost) / lambda); then for each step k, add to the nominal v and w the weighted average over the samples of the noise that was added, clamped to the model's limits
  (void)ew;
  Command out;
  out.v = st.v[0];
  out.w = st.w[0];
  for (int k = 0; k + 1 < H; ++k) {
    st.v[static_cast<std::size_t>(k)] = st.v[static_cast<std::size_t>(k + 1)];
    st.w[static_cast<std::size_t>(k)] = st.w[static_cast<std::size_t>(k + 1)];
  }
  return out;
}

struct MppiRun {
  std::vector<CarPose> trace;
  std::vector<double> yaw_rate;  // what the plant actually did, at each step
  double max_cte = 0.0;
  double final_cte = 0.0;
  double mean_cte = 0.0;
  int hits = 0;                  // poses in a blocked cell of the grid
  bool reached = false;
};

// Drives the plant model with a controller that plans with `planner` (which may be a different model).
inline MppiRun mppi_run(const MotionModel& plant, const MotionModel& planner, const MppiConfig& cfg, const std::vector<CarPose>& path,
                        const CostGrid* controller_grid, const CostGrid* judge_grid, const CarPose& start, int steps, std::uint64_t seed) {
  MppiRun run;
  MppiState st;
  st.rng.s = seed;
  CarPose pose = start;
  std::size_t idx = 0;
  double sum = 0.0;
  for (int step = 0; step < steps; ++step) {
    idx = nearest_index(path, pose.x, pose.y, idx);
    const double cte = cross_track(path, pose.x, pose.y);
    run.max_cte = std::max(run.max_cte, std::fabs(cte));
    run.final_cte = std::fabs(cte);
    sum += std::fabs(cte);
    run.trace.push_back(pose);
    if (blocked_at(judge_grid, pose.x, pose.y)) ++run.hits;
    if (idx + 1 >= path.size() && std::hypot(path.back().x - pose.x, path.back().y - pose.y) < 0.3) {
      run.reached = true;
      break;
    }
    Command c;
    if (step % cfg.replan_every == 0 || st.v.empty()) {
      c = mppi_control(planner, cfg, pose, path, idx, controller_grid, st);
    } else {
      c.v = st.v[0];
      c.w = st.w[0];
      for (std::size_t k = 0; k + 1 < st.v.size(); ++k) {
        st.v[k] = st.v[k + 1];
        st.w[k] = st.w[k + 1];
      }
    }
    const CarPose next = step_model(plant, pose, c.v, c.w, cfg.dt);
    run.yaw_rate.push_back(wrap_pi(next.theta - pose.theta) / cfg.dt);
    pose = next;
  }
  run.mean_cte = run.trace.empty() ? 0.0 : sum / static_cast<double>(run.trace.size());
  return run;
}


#endif
