// rc/drive/smooth.hpp
//
// Smoothing a path, from lesson 26-03.
//
// resample puts points an even distance apart along a polyline, the last one where the polyline ends. smooth_path moves each interior point a little towards the midpoint of its two neighbours (which straightens the path) and a little back towards where the plan had it (which stops the path wandering off), over and over until nothing moves more than a tolerance; the two ends never move. Given a cost grid it refuses any move to a cell that is inscribed or worse, so the smoother cannot pull a path into a wall. menger_curvature of three points is the curvature of the circle through them, and max_menger_curvature of a path is the sharpest of them.
//
// A path of two straight legs with a corner of 90 degrees, points 0.2 m apart, had a sharpest curvature of 7.1 per metre; after smoothing it was 3.64 against the car's limit of 1.35, in 65 sweeps. With a small obstacle just inside the corner, smoothing with no grid moved 5 points into it, and smoothing with the grid moved 0. A light smoother left the path at most 0.04 m from the plan; a heavy one moved it 0.63 m and still left a curvature of 1.55.

#ifndef RC_RC_DRIVE_SMOOTH_HPP
#define RC_RC_DRIVE_SMOOTH_HPP

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <rc/drive/hybrid_astar.hpp>
#include <vector>

namespace rc {
namespace drive {

struct SmoothConfig {
  double data_weight = 0.1;    // how hard each point is pulled back to the plan
  double smooth_weight = 0.3;  // how hard each point is pulled to its neighbours' midpoint
  double tolerance = 1e-5;     // stop when the total move in a sweep is below this, metres
  int max_sweeps = 2000;
};

struct SmoothResult {
  std::vector<CarPose> path;
  int sweeps = 0;
};

// Points an even `spacing` apart along the polyline, the first and last where it starts and ends.
inline std::vector<CarPose> resample(const std::vector<CarPose>& in, double spacing) {
  std::vector<CarPose> out;
  if (in.empty()) return out;
  out.push_back(in.front());
  double carried = 0.0;  // metres since the last point written
  for (std::size_t i = 1; i < in.size(); ++i) {
    double ax = in[i - 1].x, ay = in[i - 1].y;
    const double seg = std::hypot(in[i].x - ax, in[i].y - ay);
    double left = seg;
    while (carried + left >= spacing && seg > 0.0) {
      const double use = spacing - carried;
      ax += (in[i].x - ax) * use / left;
      ay += (in[i].y - ay) * use / left;
      left -= use;
      carried = 0.0;
      CarPose p;
      p.x = ax;
      p.y = ay;
      out.push_back(p);
    }
    carried += left;
  }
  const CarPose& last = in.back();
  if (std::hypot(out.back().x - last.x, out.back().y - last.y) > 1e-9) out.push_back(last);
  return out;
}

inline bool cell_blocked(const CostGrid* grid, double x, double y) {
  if (grid == nullptr) return false;
  return grid->at(grid->cell_i(x), grid->cell_j(y)) >= rc::perceive::kCostInscribed;
}

// Smooths the interior points of a path. The ends are fixed. With a grid, a move into a blocked cell is refused.
inline SmoothResult smooth_path(const std::vector<CarPose>& plan, const CostGrid* grid, const SmoothConfig& cfg) {
  SmoothResult r;
  r.path = plan;
  const std::size_t n = plan.size();
  if (n < 3) return r;
  for (int sweep = 0; sweep < cfg.max_sweeps; ++sweep) {
    double moved = 0.0;
    for (std::size_t i = 1; i + 1 < n; ++i) {
      const double nx = r.path[i].x + cfg.data_weight * (plan[i].x - r.path[i].x) +
                        cfg.smooth_weight * (r.path[i - 1].x + r.path[i + 1].x - 2.0 * r.path[i].x);
      const double ny = r.path[i].y + cfg.data_weight * (plan[i].y - r.path[i].y) +
                        cfg.smooth_weight * (r.path[i - 1].y + r.path[i + 1].y - 2.0 * r.path[i].y);
      if (cell_blocked(grid, nx, ny)) continue;
      moved += std::fabs(nx - r.path[i].x) + std::fabs(ny - r.path[i].y);
      r.path[i].x = nx;
      r.path[i].y = ny;
    }
    r.sweeps = sweep + 1;
    if (moved < cfg.tolerance) break;
  }
  return r;
}

// The curvature of the circle through three points (zero if two of them coincide or all are in a line).
inline double menger_curvature(const CarPose& a, const CarPose& b, const CarPose& c) {
  const double ab = std::hypot(b.x - a.x, b.y - a.y);
  const double bc = std::hypot(c.x - b.x, c.y - b.y);
  const double ca = std::hypot(a.x - c.x, a.y - c.y);
  if (ab < 1e-12 || bc < 1e-12 || ca < 1e-12) return 0.0;
  const double cross = (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
  return 2.0 * std::fabs(cross) / (ab * bc * ca);
}

inline double max_menger_curvature(const std::vector<CarPose>& path) {
  double worst = 0.0;
  for (std::size_t i = 1; i + 1 < path.size(); ++i) worst = std::max(worst, menger_curvature(path[i - 1], path[i], path[i + 1]));
  return worst;
}


}  // namespace drive
}  // namespace rc

#endif
