#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cstddef>
#include <queue>
#include <vector>

#include <rc/nav/grid.hpp>
#include <rc/nav/plan.hpp>

using rc::nav::Cell;
using rc::nav::GridPoint;
using rc::nav::OccupancyGrid;

// A frontier cell: free, and next to (up, down, left or right) a cell nobody has seen. It is where what is known meets what is not.
inline bool is_frontier(const OccupancyGrid& grid, int x, int y) {
  if (!grid.inside(x, y) || grid.classify(x, y) != Cell::free) return false;
  const int dx[4] = {1, -1, 0, 0};
  const int dy[4] = {0, 0, 1, -1};
  for (int i = 0; i < 4; ++i) {
    const int nx = x + dx[i];
    const int ny = y + dy[i];
    if (grid.inside(nx, ny) && grid.classify(nx, ny) == Cell::unknown) return true;
  }
  return false;
}

// Every frontier cell, row by row.
inline std::vector<GridPoint> find_frontiers(const OccupancyGrid& grid) {
  std::vector<GridPoint> out;
  for (int y = 0; y < grid.height(); ++y)
    for (int x = 0; x < grid.width(); ++x)
      if (is_frontier(grid, x, y)) out.push_back(GridPoint{x, y});
  return out;
}

// Frontier cells that touch, sideways or diagonally, grouped: one group is one opening onto the unknown.
inline std::vector<std::vector<GridPoint>> cluster_frontiers(const OccupancyGrid& grid) {
  const int w = grid.width();
  const int h = grid.height();
  std::vector<char> frontier(static_cast<std::size_t>(w) * h, 0);
  std::vector<char> seen(static_cast<std::size_t>(w) * h, 0);
  for (int y = 0; y < h; ++y)
    for (int x = 0; x < w; ++x) frontier[static_cast<std::size_t>(y) * w + x] = is_frontier(grid, x, y) ? 1 : 0;

  std::vector<std::vector<GridPoint>> clusters;
  for (int y = 0; y < h; ++y)
    for (int x = 0; x < w; ++x) {
      const std::size_t start = static_cast<std::size_t>(y) * w + x;
      if (!frontier[start] || seen[start]) continue;
      std::vector<GridPoint> group;
      std::queue<GridPoint> todo;
      todo.push(GridPoint{x, y});
      seen[start] = 1;
      while (!todo.empty()) {
        const GridPoint p = todo.front();
        todo.pop();
        group.push_back(p);
        for (int dy = -1; dy <= 1; ++dy)
          for (int dx = -1; dx <= 1; ++dx) {
            const int nx = p.x + dx;
            const int ny = p.y + dy;
            if (!grid.inside(nx, ny)) continue;
            const std::size_t i = static_cast<std::size_t>(ny) * w + nx;
            if (frontier[i] && !seen[i]) {
              seen[i] = 1;
              todo.push(GridPoint{nx, ny});
            }
          }
      }
      clusters.push_back(group);
    }
  return clusters;
}

enum class Explore { found, finished, unreachable };

// Where to go to see more. Only clusters of at least `min_cluster` cells count, so a single unseen speck inside a room does not send the rover to it.
// The nearest by the distance it would have to drive, counting free cells step by step, and not by the straight line, which may go through a wall.
//   finished    : no frontier of that size is left, so the map is as complete as it will get
//   unreachable : there are some, and none can be reached from `start` over free cells
//   found       : `goal` is set
inline Explore choose_frontier(const OccupancyGrid& grid, GridPoint start, std::size_t min_cluster, GridPoint& goal) {
  const int w = grid.width();
  const int h = grid.height();
  std::vector<char> wanted(static_cast<std::size_t>(w) * h, 0);
  bool any = false;
  for (const std::vector<GridPoint>& group : cluster_frontiers(grid)) {
    if (group.size() < min_cluster) continue;
    any = true;
    for (const GridPoint& p : group) wanted[static_cast<std::size_t>(p.y) * w + p.x] = 1;
  }
  if (!any) return Explore::finished;
  if (!grid.inside(start.x, start.y) || grid.classify(start.x, start.y) != Cell::free) return Explore::unreachable;

  std::vector<char> seen(static_cast<std::size_t>(w) * h, 0);
  std::queue<GridPoint> todo;
  todo.push(start);
  seen[static_cast<std::size_t>(start.y) * w + start.x] = 1;
  const int dx[4] = {1, -1, 0, 0};
  const int dy[4] = {0, 0, 1, -1};
  while (!todo.empty()) {
    const GridPoint p = todo.front();
    todo.pop();
    if (wanted[static_cast<std::size_t>(p.y) * w + p.x]) {
      goal = p;
      return Explore::found;
    }
    for (int i = 0; i < 4; ++i) {
      const int nx = p.x + dx[i];
      const int ny = p.y + dy[i];
      if (!grid.inside(nx, ny) || grid.classify(nx, ny) != Cell::free) continue;
      const std::size_t k = static_cast<std::size_t>(ny) * w + nx;
      if (seen[k]) continue;
      seen[k] = 1;
      todo.push(GridPoint{nx, ny});
    }
  }
  return Explore::unreachable;
}

#endif  // LESSON_SOLUTION_HPP
