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
  // TODO: false unless (x, y) is inside the grid and classified Cell::free.
  // Then true if any of its four neighbours (right, left, up, down) is inside the grid and Cell::unknown.
  (void)grid;
  (void)x;
  (void)y;
  return false;
}

// Every frontier cell, row by row.
inline std::vector<GridPoint> find_frontiers(const OccupancyGrid& grid) {
  // TODO: every cell for which is_frontier() is true, row by row (y outer, x inner).
  (void)grid;
  return std::vector<GridPoint>();
}

// Frontier cells that touch, sideways or diagonally, grouped: one group is one opening onto the unknown.
inline std::vector<std::vector<GridPoint>> cluster_frontiers(const OccupancyGrid& grid) {
  // TODO: group the frontier cells that touch, including diagonally (8 neighbours), by a flood fill:
  // for each frontier cell not yet seen, start a group, and keep adding unseen frontier neighbours (a queue does it).
  // Groups come out in the order their first cell appears row by row.
  (void)grid;
  return std::vector<std::vector<GridPoint>>();
}

enum class Explore { found, finished, unreachable };

// Where to go to see more. Only clusters of at least `min_cluster` cells count, so a single unseen speck inside a room does not send the rover to it.
// The nearest by the distance it would have to drive, counting free cells step by step, and not by the straight line, which may go through a wall.
//   finished    : no frontier of that size is left, so the map is as complete as it will get
//   unreachable : there are some, and none can be reached from `start` over free cells
//   found       : `goal` is set
inline Explore choose_frontier(const OccupancyGrid& grid, GridPoint start, std::size_t min_cluster, GridPoint& goal) {
  // TODO: mark every cell of every cluster with at least min_cluster cells as wanted.
  // If nothing is wanted, return Explore::finished.
  // If start is outside the grid or not free, return Explore::unreachable.
  // Otherwise a breadth first search from start over FREE cells, four neighbours (not diagonals): the first
  // cell taken off the queue that is wanted is the goal: set `goal` and return Explore::found.
  // If the search runs out, return Explore::unreachable.
  (void)grid;
  (void)start;
  (void)min_cluster;
  (void)goal;
  return Explore::unreachable;
}

#endif  // LESSON_SOLUTION_HPP
