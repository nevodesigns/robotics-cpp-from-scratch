#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cmath>
#include <cstddef>
#include <vector>

#include <rc/nav/grid.hpp>
#include <rc/nav/plan.hpp>

using rc::nav::Cell;
using rc::nav::GridPoint;
using rc::nav::OccupancyGrid;

// The same map with every obstacle grown by the robot's radius, so that a
// planner treating the robot as a point produces a route a robot with a width
// can actually drive.
//
// Growing the obstacles rather than shrinking the robot is the whole trick: it
// moves a hard geometry problem into the map once, instead of into every step
// of the search for ever.
//
// Unknown grows too, and stays unknown rather than becoming occupied. A cell
// next to unseen ground is not known to be blocked, and calling it blocked
// would make the map more certain than the evidence.
//
// Off the edge of the map counts as blocked. A robot half over the border is a
// robot somewhere the map cannot describe.
inline OccupancyGrid inflated(const OccupancyGrid& grid, double radius_metres,
                              double origin_x = 0.0, double origin_y = 0.0) {
  OccupancyGrid out(grid.width(), grid.height(), grid.resolution(), origin_x, origin_y);
  const int radius = static_cast<int>(std::ceil(radius_metres / grid.resolution()));

  for (int y = 0; y < grid.height(); ++y) {
    for (int x = 0; x < grid.width(); ++x) {
      Cell worst = Cell::free;

      for (int dy = -radius; dy <= radius && worst != Cell::occupied; ++dy) {
        for (int dx = -radius; dx <= radius; ++dx) {
          // A disc, not a square. A square inflation over-blocks the diagonals
          // by a factor of root two and closes gaps the robot would fit through.
          if (dx * dx + dy * dy > radius * radius) continue;

          const int nx = x + dx, ny = y + dy;
          if (!grid.inside(nx, ny)) {
            worst = Cell::occupied;
            break;
          }
          const Cell cell = grid.classify(nx, ny);
          if (cell == Cell::occupied) {
            worst = Cell::occupied;
            break;
          }
          if (cell == Cell::unknown) worst = Cell::unknown;
        }
      }

      // Enough observations to reach the clamp whatever the cell held before.
      // Two free observations followed by two occupied ones cancel exactly back
      // to unknown, so a helper that writes a state has to overwrite rather
      // than nudge.
      if (worst == Cell::occupied) {
        for (int i = 0; i < 12; ++i) out.observe(x, y, true);
      } else if (worst == Cell::free) {
        for (int i = 0; i < 12; ++i) out.observe(x, y, false);
      }
    }
  }
  return out;
}

// How close a world point comes to anything the map calls occupied.
//
// Measured to cell centres, so it is quantised by half a cell. That is worth
// knowing when the answer is compared against a robot radius: at a 10 cm
// resolution this figure carries 5 cm of slop and the margin has to cover it.
inline double clearance_at(const OccupancyGrid& grid, double x, double y) {
  double best = -1.0;
  for (int cy = 0; cy < grid.height(); ++cy) {
    for (int cx = 0; cx < grid.width(); ++cx) {
      if (grid.classify(cx, cy) != Cell::occupied) continue;
      const double wx = (static_cast<double>(cx) + 0.5) * grid.resolution();
      const double wy = (static_cast<double>(cy) + 0.5) * grid.resolution();
      const double distance = std::hypot(x - wx, y - wy);
      if (best < 0.0 || distance < best) best = distance;
    }
  }
  return best;
}

// The tightest point of a route, in metres.
//
// This is the number a clearance requirement is actually about, and it is not
// the planner's cost and not the follower's cross track error. A route that
// tracks perfectly and passes 5 cm from a rack is a route that hits the rack.
inline double clearance_along(const OccupancyGrid& grid,
                              const std::vector<GridPoint>& cells) {
  double worst = -1.0;
  for (const GridPoint& cell : cells) {
    const double x = (static_cast<double>(cell.x) + 0.5) * grid.resolution();
    const double y = (static_cast<double>(cell.y) + 0.5) * grid.resolution();
    const double here = clearance_at(grid, x, y);
    if (here < 0.0) continue;
    if (worst < 0.0 || here < worst) worst = here;
  }
  return worst;
}

#endif  // LESSON_SOLUTION_HPP
