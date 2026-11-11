#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <vector>

#include <rc/nav/grid.hpp>
#include <rc/nav/match.hpp>

using rc::nav::Beam;
using rc::nav::Candidate;
using rc::nav::Cell;
using rc::nav::OccupancyGrid;

// What one scan said about the map, beam by beam.
struct Surprise {
  int agree = 0;      // the beam ended on, or beside, a wall of the map
  int appeared = 0;   // the beam ended in free space: a wall that is not on the map
  int vanished = 0;   // the beam passed through a wall that is on the map
};

inline bool occupied_near(const OccupancyGrid& map, int cx, int cy, int tolerance) {
  for (int dy = -tolerance; dy <= tolerance; ++dy)
    for (int dx = -tolerance; dx <= tolerance; ++dx)
      if (map.inside(cx + dx, cy + dy) && map.classify(cx + dx, cy + dy) == Cell::occupied) return true;
  return false;
}

// Lay each beam on the map from the robot's pose, which is taken to be right.
//
// A beam that hit something is
//   an appearance if its end cell and every neighbour within one cell are known free: nothing of the map is there,
//   an agreement if its end is on or beside an occupied cell, and
//   a vanishing if, on the way there, it crossed an occupied cell more than two cells short of the end,
//   in which case it is counted as a vanishing and not also as an agreement.
// A beam that hit nothing is a vanishing if it crossed an occupied cell, and is otherwise left out.
// Cells that are unknown are neither: a beam that ends in the unknown tells nothing about whether the map is stale.
inline Surprise compare_scan(const OccupancyGrid& map, const std::vector<Beam>& scan, double no_return, const Candidate& pose) {
  // TODO: for each beam: hit = 0 < range < no_return; reach = range if it hit, otherwise no_return.
  // The end point is the pose plus reach along (theta + angle); its cell is map.cell_for(). Walk the cells from the cell of the
  // pose to the end cell (Bresenham, like OccupancyGrid::integrate_ray) and if any cell that is MORE than two cells (in x or y) short of the end cell
  // is Cell::occupied, count a vanishing and go to the next beam.
  // Otherwise, a beam that hit nothing is left out. A beam that hit: if its end cell is inside the map and occupied_near(map, x, y, 1)
  // count an agreement; else if the end cell and all eight neighbours that are inside the map are Cell::free, count an appearance.
  // An end among unknown cells counts for nothing.
  (void)map;
  (void)scan;
  (void)no_return;
  (void)pose;
  return Surprise();
}

// Whether the map has changed, which is not whether one scan disagreed. A scan counts against the map if more than `threshold`
// of its judged beams (agree + appeared + vanished) disagree, and the map is called changed after `persist` such scans in a row.
// A scan that judged no beams neither adds to the run nor ends it.
class ChangeDetector {
 public:
  ChangeDetector(double threshold, int persist) : threshold_(threshold), persist_(persist < 1 ? 1 : persist) {}

  bool update(const Surprise& s) {
    // TODO: judged = agree + appeared + vanished. If judged is more than zero, fraction = (appeared + vanished) / judged;
    // if the fraction is more than threshold_ add one to streak_, otherwise streak_ goes back to zero.
    // A scan that judged nothing leaves streak_ alone. Return changed().
    (void)s;
    return changed();
  }

  bool changed() const { return streak_ >= persist_; }
  double threshold() const { return threshold_; }
  int streak() const { return streak_; }
  void reset() { streak_ = 0; }

 private:
  double threshold_ = 0.05;
  int persist_ = 3;
  int streak_ = 0;
};

#endif  // LESSON_SOLUTION_HPP
