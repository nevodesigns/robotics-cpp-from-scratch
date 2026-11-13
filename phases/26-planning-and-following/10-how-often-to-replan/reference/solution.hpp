#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cmath>
#include <cstddef>
#include <rc/drive/follow.hpp>
#include <rc/drive/hybrid_astar.hpp>
#include <vector>

using namespace rc::drive;

enum ReplanReason { kNoReplan = 0, kBlocked = 1, kOffPath = 2, kPeriodic = 3, kGoalChanged = 4 };

struct ReplanConfig {
  double check_ahead = 3.0;   // metres of the path ahead that are checked against the grid
  double off_path = 0.6;      // metres from the path that makes a new plan
  double period = 10.0;       // seconds after which the plan is remade anyway
  double min_interval = 1.0;  // seconds between plans, except for the reasons that bypass it
  bool blocked_bypasses = true;
};

// The index of the first point of the path, from `from` and within `ahead` metres along it, that lies on a blocked cell; or -1.
inline int first_blocked(const CostGrid& grid, const std::vector<CarPose>& path, std::size_t from, double ahead) {
  double along = 0.0;
  for (std::size_t i = from; i < path.size(); ++i) {
    if (i > from) along += std::hypot(path[i].x - path[i - 1].x, path[i].y - path[i - 1].y);
    if (along > ahead) break;
    if (grid.at(grid.cell_i(path[i].x), grid.cell_j(path[i].y)) >= rc::perceive::kCostInscribed) return static_cast<int>(i);
  }
  return -1;
}

// Whether to plan again now, and why.
inline ReplanReason should_replan(const ReplanConfig& cfg, double now, double last_plan, const CarPose& pose, const std::vector<CarPose>& path,
                                  std::size_t idx, const CostGrid& grid, bool goal_changed) {
  const bool cooling = now - last_plan < cfg.min_interval;
  bool blocked = false, off = false, due = false;
  blocked = !path.empty() && first_blocked(grid, path, idx, cfg.check_ahead) >= 0;
  off = path.size() > 1 && std::fabs(cross_track(path, pose.x, pose.y)) > cfg.off_path;
  due = now - last_plan >= cfg.period;
  if (goal_changed) return kGoalChanged;
  if (blocked && (!cooling || cfg.blocked_bypasses)) return kBlocked;
  if (cooling) return kNoReplan;
  if (off) return kOffPath;
  if (due) return kPeriodic;
  return kNoReplan;
}

// Where the vehicle will be on the path after `latency` seconds at `speed`, from the point nearest to it: the place a new plan should start.
inline CarPose predicted_pose(const CarPose& pose, const std::vector<CarPose>& path, std::size_t idx, double speed, double latency) {
  if (path.empty()) return pose;
  double left = speed * latency;
  std::size_t i = idx < path.size() ? idx : path.size() - 1;
  CarPose p = path[i];
  while (i + 1 < path.size()) {
    const double d = std::hypot(path[i + 1].x - path[i].x, path[i + 1].y - path[i].y);
    if (d >= left) {
      const double u = d > 1e-12 ? left / d : 0.0;
      p.x = path[i].x + u * (path[i + 1].x - path[i].x);
      p.y = path[i].y + u * (path[i + 1].y - path[i].y);
      return p;
    }
    left -= d;
    ++i;
    p = path[i];
  }
  return p;
}


#endif
