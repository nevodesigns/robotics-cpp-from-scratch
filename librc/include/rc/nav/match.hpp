// rc/nav/match.hpp
//
// Matching a range scan to a map, from lesson 16-13, graduated.
//
// A pose is scored by the share of the beams that hit something and end on a wall of the map (or within a tolerance of one); beams with no return are left out.
// A search tries every pose in a box round the odometry's guess and takes the best, with the one nearest the guess winning a tie, and counts how many poses scored within 0.005 of the best.
//
// A perfect scan scored 1.000 at its own pose and 0.787, 0.656, 0.639 and 0.607 at 5, 10, 20 and 40 cm off. Odometry that had drifted 0.250 m was brought to 0.000 m. Ranges noisy by 3 cm
// scored 0.492 at a tolerance of 0 cells, 0.951 at 1 and 0.984 at 2.
//
// A 30 m corridor tied 41 poses with the best along its length where a room with a block tied 1, and an empty rectangular room scored its true pose and its mirror through the centre at 1.000 each.

#ifndef RC_NAV_MATCH_HPP
#define RC_NAV_MATCH_HPP

#include <cmath>
#include <cstddef>
#include <vector>
#include <rc/nav/grid.hpp>

namespace rc {
namespace nav {

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
  int used = 0;
  int on_wall = 0;
  for (const Beam& b : scan) {
    if (b.range <= 0.0 || b.range >= no_return) continue;
    ++used;
    const double ex = pose.x + b.range * std::cos(pose.theta + b.angle);
    const double ey = pose.y + b.range * std::sin(pose.theta + b.angle);
    const rc::nav::CellIndex c = map.cell_for(ex, ey);
    if (c.inside && near_occupied(map, c.x, c.y, tolerance)) ++on_wall;
  }
  return used > 0 ? static_cast<double>(on_wall) / used : 0.0;
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
  if (xy_step <= 0.0 || theta_step <= 0.0) return result;
  const int nxy = static_cast<int>(std::floor(xy_range / xy_step + 1e-9));
  const int nth = static_cast<int>(std::floor(theta_range / theta_step + 1e-9));

  std::vector<double> scores;
  std::vector<Candidate> poses;
  double best = -1.0;
  for (int it = -nth; it <= nth; ++it)
    for (int iy = -nxy; iy <= nxy; ++iy)
      for (int ix = -nxy; ix <= nxy; ++ix) {
        Candidate c;
        c.x = guess.x + ix * xy_step;
        c.y = guess.y + iy * xy_step;
        c.theta = guess.theta + it * theta_step;
        const double s = score_pose(map, scan, no_return, c, tolerance);
        scores.push_back(s);
        poses.push_back(c);
        if (s > best) best = s;
      }

  double nearest = 1e18;
  for (std::size_t i = 0; i < poses.size(); ++i) {
    if (scores[i] >= best - 0.005) ++result.tied;
    if (scores[i] == best) {
      const double d = std::hypot(poses[i].x - guess.x, poses[i].y - guess.y) + std::fabs(poses[i].theta - guess.theta);
      if (d < nearest) {
        nearest = d;
        result.best = poses[i];
      }
    }
  }
  result.score = best < 0.0 ? 0.0 : best;
  return result;
}

}  // namespace nav
}  // namespace rc

#endif  // RC_NAV_MATCH_HPP
