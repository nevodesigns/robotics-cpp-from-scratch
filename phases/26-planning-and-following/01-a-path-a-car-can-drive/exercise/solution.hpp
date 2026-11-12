#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <map>
#include <queue>
#include <rc/perceive/costmap.hpp>
#include <vector>

using rc::perceive::CostGrid;

struct CarPose {
  double x = 0.0;
  double y = 0.0;
  double theta = 0.0;
};

struct HybridConfig {
  double wheelbase = 0.33;
  double max_steer = 0.4189;     // radians
  double step = 0.6;             // metres of arc for each move
  double resolution = 0.25;      // metres, of the grid that keeps one pose to a cell
  int theta_bins = 24;
  double steer_penalty = 0.2;    // per move, for steering at all
  double change_penalty = 0.4;   // per move, for changing the steering from the move before
  double reverse_penalty = 2.0;  // per move, for going backwards
  double cost_weight = 0.02;     // per move, per unit of cell cost on the way
  bool allow_reverse = false;
  int max_expansions = 30000;
  double goal_xy = 0.3;
  double goal_theta = 0.35;
};

struct HybridResult {
  bool found = false;
  std::vector<CarPose> path;
  int expansions = 0;
  double length = 0.0;
};

struct HybridNode {
  CarPose pose;
  double g = 0.0;
  int parent = -1;
  double steer = 0.0;
  int direction = 1;
};

// The pose after driving an arc of length `ds` (negative to reverse) at a steering angle, with the bicycle model.
inline CarPose drive_arc(const CarPose& p, double steer, double ds, double wheelbase) {
  CarPose q = p;
  const double rate = std::tan(steer) / wheelbase;  // radians of heading per metre
  const double mid = p.theta + 0.5 * rate * ds;
  q.x += ds * std::cos(mid);
  q.y += ds * std::sin(mid);
  q.theta += rate * ds;
  return q;
}

inline double wrap_pi(double a) { return std::atan2(std::sin(a), std::cos(a)); }

// Is the arc from a pose to the next free: every point along it on a cell of cost below inscribed? Adds the cost of the cells to `cost_sum`.
inline bool arc_is_free(const CostGrid& grid, const CarPose& from, double steer, double ds, double wheelbase, double* cost_sum) {
  const int n = 6;
  double sum = 0.0;
  for (int k = 1; k <= n; ++k) {
    const CarPose q = drive_arc(from, steer, ds * k / n, wheelbase);
    const unsigned char c = grid.at(grid.cell_i(q.x), grid.cell_j(q.y));
    if (c >= rc::perceive::kCostInscribed) return false;
    sum += c;
  }
  if (cost_sum != nullptr) *cost_sum = sum / n;
  return true;
}

inline long node_key(const HybridConfig& c, const CarPose& p) {
  const long ix = static_cast<long>(std::floor(p.x / c.resolution));
  const long iy = static_cast<long>(std::floor(p.y / c.resolution));
  double th = std::fmod(p.theta, 6.283185307179586);
  if (th < 0.0) th += 6.283185307179586;
  const long it = static_cast<long>(th / 6.283185307179586 * c.theta_bins) % c.theta_bins;
  return ((ix + 100000L) * 200003L + (iy + 100000L)) * 64L + it;
}

// Searches for a path from start to goal on the grid.
inline HybridResult hybrid_astar(const CostGrid& grid, const CarPose& start, const CarPose& goal, const HybridConfig& cfg) {
  HybridResult result;
  std::vector<HybridNode> nodes;
  std::map<long, double> best_g;
  using Entry = std::pair<double, int>;
  std::priority_queue<Entry, std::vector<Entry>, std::greater<Entry> > open;
  HybridNode first;
  first.pose = start;
  nodes.push_back(first);
  best_g[node_key(cfg, start)] = 0.0;
  open.push(Entry(std::hypot(goal.x - start.x, goal.y - start.y), 0));
  const double steers[5] = {-cfg.max_steer, -0.5 * cfg.max_steer, 0.0, 0.5 * cfg.max_steer, cfg.max_steer};
  (void)open;  // TODO: take the cheapest open node; stop if it is near the goal in position and heading and walk back through the parents; otherwise, for each steering (and reverse if allowed) whose arc is free, make the new pose, price the move, and keep the node if it is cheaper than any before in its cell of the coarse grid
  (void)nodes;
  (void)steers;
  return result;
}

// The largest curvature (radians of heading per metre) between neighbouring poses of a path.
inline double max_curvature(const std::vector<CarPose>& path) {
  double worst = 0.0;
  for (std::size_t i = 1; i < path.size(); ++i) {
    const double ds = std::hypot(path[i].x - path[i - 1].x, path[i].y - path[i - 1].y);
    if (ds < 1e-9) continue;
    worst = std::max(worst, std::fabs(wrap_pi(path[i].theta - path[i - 1].theta)) / ds);
  }
  return worst;
}


#endif
