// rc/nav/change.hpp
//
// Noticing that the map is out of date, from lesson 16-15, graduated.
//
// Each beam of a scan, laid on the map from the robot's pose, agrees (ends on or beside a wall), shows an appearance (ends in free space: a wall that is not on the map) or a vanishing (passes through a
// wall that is on the map). The map is called changed only when more than a threshold share of the judged beams disagree on several scans in a row.
//
// A scan of the map as it is gave 82 agree, 0 appeared, 0 vanished. A door that had closed gave 82, 9 and 0; a door that had opened, 80, 0 and 11. With ranges noisy by 3 cm, unchanged scans were
// called different 0.800, 0.484, 0.072, 0.004 and 0.000 of the time at thresholds of 0.01, 0.02, 0.04, 0.08 and 0.15, and closed-door scans 1.000, 1.000, 1.000, 1.000 and 0.012.
//
// Compared with the map as it was, a closed door was noticed at the third scan; compared with the map being repaired by the same scans, never, though the repaired map's doorway cell became a wall at scan 11.

#ifndef RC_NAV_CHANGE_HPP
#define RC_NAV_CHANGE_HPP

#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <vector>
#include <rc/nav/grid.hpp>
#include <rc/nav/match.hpp>

namespace rc {
namespace nav {

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
  Surprise s;
  const rc::nav::CellIndex from = map.cell_for(pose.x, pose.y);
  for (const Beam& b : scan) {
    const bool hit = b.range > 0.0 && b.range < no_return;
    const double reach = hit ? b.range : no_return;
    const double ex = pose.x + reach * std::cos(pose.theta + b.angle);
    const double ey = pose.y + reach * std::sin(pose.theta + b.angle);
    const rc::nav::CellIndex to = map.cell_for(ex, ey);

    // Walk the cells from the robot to the end of the beam.
    int x = from.x;
    int y = from.y;
    const int dx = std::abs(to.x - x);
    const int dy = -std::abs(to.y - y);
    const int step_x = x < to.x ? 1 : -1;
    const int step_y = y < to.y ? 1 : -1;
    int error = dx + dy;
    bool crossed = false;
    while (x != to.x || y != to.y) {
      const int doubled = 2 * error;
      if (doubled >= dy) {
        error += dy;
        x += step_x;
      }
      if (doubled <= dx) {
        error += dx;
        y += step_y;
      }
      const bool near_end = std::abs(x - to.x) <= 2 && std::abs(y - to.y) <= 2;
      if (!near_end && map.inside(x, y) && map.classify(x, y) == Cell::occupied) {
        crossed = true;
        break;
      }
    }
    if (crossed) {
      ++s.vanished;
      continue;
    }
    if (!hit) continue;
    if (!to.inside) continue;
    if (occupied_near(map, to.x, to.y, 1)) {
      ++s.agree;
    } else {
      bool all_free = true;
      for (int oy = -1; oy <= 1 && all_free; ++oy)
        for (int ox = -1; ox <= 1; ++ox)
          if (map.inside(to.x + ox, to.y + oy) && map.classify(to.x + ox, to.y + oy) != Cell::free) all_free = false;
      if (all_free) ++s.appeared;
    }
  }
  return s;
}

// Whether the map has changed, which is not whether one scan disagreed. A scan counts against the map if more than `threshold`
// of its judged beams (agree + appeared + vanished) disagree, and the map is called changed after `persist` such scans in a row.
// A scan that judged no beams neither adds to the run nor ends it.
class ChangeDetector {
 public:
  ChangeDetector(double threshold, int persist) : threshold_(threshold), persist_(persist < 1 ? 1 : persist) {}

  bool update(const Surprise& s) {
    const int judged = s.agree + s.appeared + s.vanished;
    if (judged > 0) {
      const double fraction = static_cast<double>(s.appeared + s.vanished) / judged;
      if (fraction > threshold_) ++streak_;
      else streak_ = 0;
    }
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

}  // namespace nav
}  // namespace rc

#endif  // RC_NAV_CHANGE_HPP
