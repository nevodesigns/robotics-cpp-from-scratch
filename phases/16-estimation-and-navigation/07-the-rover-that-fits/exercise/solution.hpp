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

// TODO 1: the same map with every obstacle grown by the robot's radius.
//
// For each cell, look at every cell within `radius_metres` of it and take the
// worst thing you find: occupied beats unknown beats free. Write that into the
// output grid.
//
// Four details, each of which is a decision rather than an implementation note.
//
//   A disc, not a square. Skip a neighbour when dx*dx + dy*dy is greater than
//   the radius in cells squared. A square inflation over-blocks the diagonals
//   by a factor of root two and closes gaps the robot would fit through.
//
//   Round the radius up to whole cells with std::ceil. Half a cell does not
//   exist, and rounding down would produce a map that says the robot fits when
//   it does not. Inflation is therefore conservative by up to one cell, and
//   that belongs in the margin rather than being a surprise.
//
//   Off the edge of the map is occupied. A robot half over the border is a
//   robot somewhere the map cannot describe.
//
//   Unknown grows as unknown, not as occupied. A cell beside unseen ground is
//   not known to be blocked, and saying it is makes the map more certain than
//   the evidence.
//
// To write a state into a cell, observe it enough times to reach the clamp: two
// free observations followed by two occupied ones cancel exactly back to
// unknown, so a helper that sets a state has to overwrite rather than nudge.
// Twelve is plenty.
inline OccupancyGrid inflated(const OccupancyGrid& grid, double radius_metres,
                              double origin_x = 0.0, double origin_y = 0.0) {
  (void)radius_metres;
  return OccupancyGrid(grid.width(), grid.height(), grid.resolution(), origin_x, origin_y);
}

// TODO 2: how close a world point comes to anything the map calls occupied.
//
// The distance to the nearest occupied cell's centre, or a negative number if
// the map has no occupied cells at all.
//
// Measuring to cell centres quantises the answer by half a cell. At a 10 cm
// resolution that is 5 cm of slop in a number that gets compared against a
// robot radius, so the margin has to cover it.
inline double clearance_at(const OccupancyGrid& grid, double x, double y) {
  (void)grid;
  (void)x;
  (void)y;
  return -1.0;
}

// TODO 3: the tightest point of a route, in metres.
//
// The smallest clearance_at over every cell of the path. Skip cells where
// clearance_at has nothing to report, and return a negative number if none of
// them did.
//
// This is the number a clearance requirement is actually about, and it is
// neither the planner's cost nor the follower's cross track error. A route that
// tracks perfectly and passes 5 cm from a rack is a route that hits the rack.
inline double clearance_along(const OccupancyGrid& grid,
                              const std::vector<GridPoint>& cells) {
  (void)grid;
  (void)cells;
  return -1.0;
}

#endif  // LESSON_SOLUTION_HPP
