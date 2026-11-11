#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cmath>
#include <cstddef>
#include <vector>

#include <rc/nav/grid.hpp>

using rc::nav::Cell;
using rc::nav::OccupancyGrid;

// One range reading: the angle it was taken at, relative to the way the robot faces, and the distance. A range of zero or less, or
// at least `no_return`, means the beam hit nothing.
struct Beam {
  double angle = 0.0;
  double range = 0.0;
};

struct Candidate {
  double x = 0.0;
  double y = 0.0;
  double theta = 0.0;
};

// Is any cell within `tolerance` cells (a square) of this one occupied?
inline bool near_occupied(const OccupancyGrid& map, int cx, int cy, int tolerance) {
  for (int dy = -tolerance; dy <= tolerance; ++dy)
    for (int dx = -tolerance; dx <= tolerance; ++dx)
      if (map.inside(cx + dx, cy + dy) && map.classify(cx + dx, cy + dy) == Cell::occupied) return true;
  return false;
}

// What share of the beams that hit something end on a wall of the map, if the robot is at (x, y) facing theta.
// A beam counts as ending on a wall if its end point is in, or within `tolerance` cells of, an occupied cell.
// Beams with no return are left out, and a scan with none gives zero.
inline double score_pose(const OccupancyGrid& map, const std::vector<Beam>& scan, double no_return, const Candidate& pose, int tolerance) {
  // TODO: for each beam with 0 < range < no_return, count it as used. Its end point is
  // (pose.x + range * cos(pose.theta + angle), pose.y + range * sin(pose.theta + angle)).
  // Find its cell with map.cell_for(ex, ey); if the cell is inside the map and near_occupied(map, c.x, c.y, tolerance)
  // the beam is on a wall. Return on_wall / used, or 0 if no beam was used.
  (void)map;
  (void)scan;
  (void)no_return;
  (void)pose;
  (void)tolerance;
  return -1.0;
}

struct MatchResult {
  Candidate best;           // the best fitting pose, and of equals the one nearest the guess
  double score = 0.0;
  int tied = 0;             // how many poses scored within 0.005 of the best
};

// Try every pose in a box around `guess`: x and y within `xy_range` in steps of `xy_step`, theta within `theta_range` in steps of `theta_step`.
// The best score wins, and of poses with the same score the one nearest the guess wins, so that a map that cannot tell them apart
// does not move the robot for no reason. Also counts how many poses came within 0.005 of the best score.
inline MatchResult search_pose(const OccupancyGrid& map, const std::vector<Beam>& scan, double no_return, const Candidate& guess,
                               double xy_range, double xy_step, double theta_range, double theta_step, int tolerance) {
  MatchResult result;
  result.best = guess;
  // TODO: result.best starts as the guess. For a step of zero or less, return that.
  // nxy = floor(xy_range / xy_step), nth = floor(theta_range / theta_step) (add 1e-9 before flooring).
  // Score every pose guess + (ix * xy_step, iy * xy_step, it * theta_step) for ix and iy in -nxy..nxy and it in -nth..nth.
  // The best score wins; among poses with exactly the best score the one nearest the guess wins
  // (distance = hypot of x and y offsets plus the absolute theta offset).
  // result.tied counts the poses scoring within 0.005 of the best; result.score is the best score.
  (void)map;
  (void)scan;
  (void)no_return;
  (void)guess;
  (void)xy_range;
  (void)xy_step;
  (void)theta_range;
  (void)theta_step;
  (void)tolerance;
  return result;
}

#endif  // LESSON_SOLUTION_HPP
