// rc/drive/replan.hpp
//
// How often to replan, from lesson 26-10.
//
// first_blocked looks along a path from the vehicle's place, for a given distance, and gives the index of the first point on a blocked cell of the latest grid (or minus one). should_replan answers, for this moment, whether to plan again and why: a changed goal first; then a blocked point within check_ahead metres; then, unless the last plan was less than min_interval seconds ago, the vehicle being further than off_path metres from the path, or a period having passed. A blocked path may be allowed to bypass the cooldown, because waiting costs a collision or a stop. predicted_pose gives where the vehicle will be on the path after the time a plan takes to arrive, which is where the new plan should start.
//
// In a field with a wall found only when the vehicle was 4 m from it, a vehicle that never replanned stopped in front of it and never arrived; one that replanned for the reasons arrived in 20.9 s with 5 plans; one that planned every 0.2 s arrived in 21.4 s with 86. With a plan taking 2 s, a look-ahead of 0.6 m left the vehicle standing for 2.1 s and one of 3 m for 0.3 s. A plan that took 1 s, begun from where the vehicle was, started 1.16 m behind it when it arrived, which is more than the 0.6 m of the off-path reason, so the vehicle planned again and again: 83 plans in 90 s and it never arrived; begun from where the vehicle would be, the plan started 0.06 m from it and the vehicle arrived in 20.1 s with 5 plans.

#ifndef RC_RC_DRIVE_REPLAN_HPP
#define RC_RC_DRIVE_REPLAN_HPP

#include <cmath>
#include <cstddef>
#include <rc/drive/follow.hpp>
#include <rc/drive/hybrid_astar.hpp>
#include <vector>

namespace rc {
namespace drive {

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


}  // namespace drive
}  // namespace rc

#endif
