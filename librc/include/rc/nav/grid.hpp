// rc/nav/grid.hpp
//
// A map that can be told it was wrong, from lesson 16-05.
//
// Cells hold log odds rather than probabilities. That is not a performance
// decision: multiplying probabilities together drives a cell to exactly 1.0
// after forty four consistent observations, because one minus it falls below
// what a double can hold beside one, and from there no amount of contrary
// evidence will ever move it. A parked van that drives away leaves a wall in
// the map for good.
//
//   observations   1 - p, multiplied   misses to recover
//   40                     1.887e-15                  41
//   43                     1.110e-16                  44
//   44                     0.000e+00               never
//   2000                   0.000e+00               never
//
// In log odds an observation is an addition and the sum is clamped, so how
// stubborn a cell may become is bounded. Measured, a cell recovers in five
// contrary observations whether it was confirmed ten times or two thousand.

#ifndef RC_NAV_GRID
#define RC_NAV_GRID

#include <cmath>
#include <cstddef>
#include <vector>

namespace rc {
namespace nav {

// What the map is willing to say about one square of the world.
//
// Three answers, not two. A cell nobody has looked at is not free, and the
// difference between those is the difference between a robot that waits and one
// that drives into a stairwell.
enum class Cell {
  unknown,
  free,
  occupied,
};

inline const char* describe(Cell cell) {
  switch (cell) {
    case Cell::unknown: return "unknown";
    case Cell::free: return "free";
    case Cell::occupied: return "occupied";
  }
  return "unknown";
}

// Where a point in the world falls in the grid, and whether it falls in it.
struct CellIndex {
  int x = 0;
  int y = 0;
  bool inside = false;
};

// A grid of evidence about what is where.
//
// Cells hold log odds rather than probabilities, and that is not a performance
// decision. Multiplying probabilities together drives a cell to exactly 1.0
// after forty four consistent observations, at which point one minus it is zero
// and no amount of contrary evidence will ever move it again: a parked car that
// drives away leaves a wall in the map for ever.
//
// In log odds an observation is an addition, and clamping the sum bounds how
// stubborn a cell is allowed to become. Measured, with the clamp at plus and
// minus four, a cell recovers in five contrary observations whether it was
// confirmed ten times or two thousand.
class OccupancyGrid {
 public:
  // Log odds of one observation, from a sensor right seven times in ten.
  static constexpr double kHit = 0.8472978603872034;    // log(0.7 / 0.3)
  static constexpr double kMiss = -0.8472978603872034;

  // How sure a cell is allowed to get. This is the whole defence against a map
  // that cannot be corrected.
  static constexpr double kClamp = 4.0;

  // How much evidence before the map will commit to an answer.
  static constexpr double kDecided = 1.0;

  OccupancyGrid(int width, int height, double resolution, double origin_x, double origin_y)
      : width_(width > 0 ? width : 1),
        height_(height > 0 ? height : 1),
        resolution_(resolution > 0.0 ? resolution : 0.05),
        origin_x_(origin_x),
        origin_y_(origin_y),
        cells_(static_cast<std::size_t>(width_) * static_cast<std::size_t>(height_), 0.0) {}

  int width() const { return width_; }
  int height() const { return height_; }
  double resolution() const { return resolution_; }

  bool inside(int x, int y) const {
    return x >= 0 && y >= 0 && x < width_ && y < height_;
  }

  // Which cell a world point falls in.
  //
  // std::floor, not a cast to int. A cast truncates toward zero, which sends
  // -0.05 and +0.05 to the same cell and makes the cell at the origin twice as
  // wide as every other one. On a map whose origin is in the middle, which is
  // the usual arrangement, that is a seam down the centre of the world.
  CellIndex cell_for(double x, double y) const {
    CellIndex index;
    index.x = static_cast<int>(std::floor((x - origin_x_) / resolution_));
    index.y = static_cast<int>(std::floor((y - origin_y_) / resolution_));
    index.inside = inside(index.x, index.y);
    return index;
  }

  double log_odds(int x, int y) const {
    if (!inside(x, y)) return 0.0;
    return cells_[index_of(x, y)];
  }

  double probability(int x, int y) const {
    const double l = log_odds(x, y);
    return 1.0 - 1.0 / (1.0 + std::exp(l));
  }

  Cell classify(int x, int y) const {
    const double l = log_odds(x, y);
    if (l >= kDecided) return Cell::occupied;
    if (l <= -kDecided) return Cell::free;
    return Cell::unknown;
  }

  // One observation of one cell. Addition, then clamp.
  void observe(int x, int y, bool occupied) {
    if (!inside(x, y)) return;
    double& cell = cells_[index_of(x, y)];
    cell += occupied ? kHit : kMiss;
    if (cell > kClamp) cell = kClamp;
    if (cell < -kClamp) cell = -kClamp;
  }

  // One range reading: everything the beam passed through is free, and where it
  // stopped is occupied.
  //
  // `hit` is whether the beam stopped on something. A reading that ran out to
  // the sensor's maximum without returning says only that the space is empty;
  // marking its far end occupied paints a ring of walls at exactly max range
  // around every place the robot has ever stood.
  void integrate_ray(double from_x, double from_y, double to_x, double to_y, bool hit) {
    const CellIndex start = cell_for(from_x, from_y);
    const CellIndex end = cell_for(to_x, to_y);

    // Bresenham. Every cell the line passes through, in order, integers only.
    int x = start.x, y = start.y;
    const int dx = std::abs(end.x - x), dy = -std::abs(end.y - y);
    const int step_x = x < end.x ? 1 : -1, step_y = y < end.y ? 1 : -1;
    int error = dx + dy;

    while (true) {
      if (x == end.x && y == end.y) break;
      observe(x, y, false);   // the beam passed through here

      const int doubled = 2 * error;
      if (doubled >= dy) {
        error += dy;
        x += step_x;
      }
      if (doubled <= dx) {
        error += dx;
        y += step_y;
      }
    }

    // The endpoint is the only cell that gets the other answer, and only when
    // the beam actually stopped there. Marking it free along with the rest of
    // the ray erases the obstacle the reading just found.
    if (hit) observe(end.x, end.y, true);
  }

 private:
  std::size_t index_of(int x, int y) const {
    return static_cast<std::size_t>(y) * static_cast<std::size_t>(width_) +
           static_cast<std::size_t>(x);
  }

  int width_ = 1;
  int height_ = 1;
  double resolution_ = 0.05;
  double origin_x_ = 0.0;
  double origin_y_ = 0.0;
  std::vector<double> cells_;
};

}  // namespace nav
}  // namespace rc

#endif  // RC_NAV_GRID
