#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <vector>

#include <rc/nav/grid.hpp>
#include <rc/nav/plan.hpp>

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
  // TODO: walk from a to b one cell at a time. The start cell must be free (cell_is_free()).
  // Bresenham's walk: dx = |b.x - x|, dy = -|b.y - y|, step_x and step_y are +1 or -1 towards b, error = dx + dy.
  // Until you reach b: doubled = 2 * error; move_x = doubled >= dy; move_y = doubled <= dx.
  // If move_x: error += dy, x += step_x. If move_y: error += dx, y += step_y.
  // Each new cell must be free. A step that moved on BOTH axes must also have both cells it passed between free:
  // (x - step_x, y) and (x, y - step_y), or the line has slipped between two diagonal walls.
  (void)grid;
  (void)a;
  (void)b;
  return false;
}

// Keep only the cells of `path` that are needed: from each kept cell, jump to the furthest later cell that can be reached
// in a clear straight line, and keep that one. The first and last are always kept. An empty path stays empty.
inline std::vector<GridPoint> shorten(const OccupancyGrid& grid, const std::vector<GridPoint>& path) {
  // TODO: an empty path stays empty. Keep path.front(). From the last kept cell (the anchor), look from the END of the
  // path backwards for the furthest cell with line_is_clear() from the anchor; if none beyond the next cell is, take the next cell.
  // Keep that cell and make it the new anchor. Repeat until the last cell is kept.
  (void)grid;
  (void)path;
  return std::vector<GridPoint>();
}

// How many times the heading changes by more than `threshold` radians along the path.
// Cells that do not move (a repeated point) are skipped rather than counted as a heading.
inline int count_turns(const std::vector<GridPoint>& path, double threshold) {
  // TODO: for each pair of neighbouring cells the heading is atan2(dy, dx); skip a pair that does not move.
  // Count a turn whenever the heading differs from the last one by more than `threshold` radians,
  // taking the shorter way round (a difference over pi is 2 pi minus it).
  (void)path;
  (void)threshold;
  return -1;
}

#endif  // LESSON_SOLUTION_HPP
