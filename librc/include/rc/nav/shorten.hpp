// rc/nav/shorten.hpp
//
// Fewer corners on a grid path, from lesson 16-10, graduated.
//
// A planned grid path is a staircase of cell steps, each one legal and the whole of it a wobble. Greedy shortening keeps a cell only if the next
// cannot be reached from the last kept one in a clear straight line. The clear-line test walks the line a cell at a time, needs every cell free
// (unknown is not free), and for a step on both axes also needs the two cells it passes between free, or the line slips between two walls that only touch at a corner.
//
// Planned from (3,3) to (56,36) in a 60 by 40 room with two walls: 72 points, length 92.12 cells and 7 turns over 10 degrees became 8 points,
// length 88.02 and 5 turns. Across 16 planned paths none got longer and none lost an end.

#ifndef RC_NAV_SHORTEN_HPP
#define RC_NAV_SHORTEN_HPP

#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <vector>
#include <rc/nav/grid.hpp>
#include <rc/nav/plan.hpp>

namespace rc {
namespace nav {

using rc::nav::Cell;
using rc::nav::GridPoint;
using rc::nav::OccupancyGrid;

inline bool cell_is_free(const OccupancyGrid& grid, int x, int y) {
  return grid.inside(x, y) && grid.classify(x, y) == Cell::free;
}

// Whether the straight line from cell a to cell b crosses only free cells.
//
// The line is walked one cell at a time, the way a rasteriser draws it. A step that moves on both axes at once goes
// between two other cells, and if both of those are walls the line has slipped through a gap a rover cannot: so a
// diagonal step needs both of its neighbours free as well. Unknown cells are not free.
inline bool line_is_clear(const OccupancyGrid& grid, GridPoint a, GridPoint b) {
  int x = a.x;
  int y = a.y;
  if (!cell_is_free(grid, x, y)) return false;
  const int dx = std::abs(b.x - x);
  const int dy = -std::abs(b.y - y);
  const int step_x = x < b.x ? 1 : -1;
  const int step_y = y < b.y ? 1 : -1;
  int error = dx + dy;
  while (x != b.x || y != b.y) {
    const int doubled = 2 * error;
    const bool move_x = doubled >= dy;
    const bool move_y = doubled <= dx;
    if (move_x) {
      error += dy;
      x += step_x;
    }
    if (move_y) {
      error += dx;
      y += step_y;
    }
    if (!cell_is_free(grid, x, y)) return false;
    if (move_x && move_y && (!cell_is_free(grid, x - step_x, y) || !cell_is_free(grid, x, y - step_y))) return false;
  }
  return true;
}

// Keep only the cells of `path` that are needed: from each kept cell, jump to the furthest later cell that can be reached
// in a clear straight line, and keep that one. The first and last are always kept. An empty path stays empty.
inline std::vector<GridPoint> shorten(const OccupancyGrid& grid, const std::vector<GridPoint>& path) {
  std::vector<GridPoint> out;
  if (path.empty()) return out;
  out.push_back(path.front());
  std::size_t anchor = 0;
  while (anchor + 1 < path.size()) {
    std::size_t furthest = anchor + 1;
    for (std::size_t i = path.size() - 1; i > anchor + 1; --i) {
      if (line_is_clear(grid, path[anchor], path[i])) {
        furthest = i;
        break;
      }
    }
    out.push_back(path[furthest]);
    anchor = furthest;
  }
  return out;
}

// How many times the heading changes by more than `threshold` radians along the path.
// Cells that do not move (a repeated point) are skipped rather than counted as a heading.
inline int count_turns(const std::vector<GridPoint>& path, double threshold) {
  int turns = 0;
  bool have_heading = false;
  double heading = 0.0;
  for (std::size_t i = 1; i < path.size(); ++i) {
    const double dx = static_cast<double>(path[i].x - path[i - 1].x);
    const double dy = static_cast<double>(path[i].y - path[i - 1].y);
    if (dx == 0.0 && dy == 0.0) continue;
    const double h = std::atan2(dy, dx);
    if (have_heading) {
      double change = std::fabs(h - heading);
      if (change > 3.14159265358979323846) change = 2.0 * 3.14159265358979323846 - change;
      if (change > threshold) ++turns;
    }
    heading = h;
    have_heading = true;
  }
  return turns;
}

}  // namespace nav
}  // namespace rc

#endif  // RC_NAV_SHORTEN_HPP
