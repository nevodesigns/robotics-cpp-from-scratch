#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cmath>
#include <cstddef>
#include <vector>

// What the map is willing to say about one square of the world. Three answers,
// not two: a cell nobody has looked at is not free.
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

struct CellIndex {
  int x = 0;
  int y = 0;
  bool inside = false;
};

class OccupancyGrid {
 public:
  static constexpr double kHit = 0.8472978603872034;    // log(0.7 / 0.3)
  static constexpr double kMiss = -0.8472978603872034;
  static constexpr double kClamp = 4.0;
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

  // TODO 1: which cell a world point falls in.
  //
  // Subtract the origin, divide by the resolution, and take the floor. Set
  // inside from inside().
  //
  // std::floor, not a cast to int. A cast truncates toward zero, so -0.05 and
  // +0.05 land in the same cell and the cell at the origin comes out twice as
  // wide as every other one. The test in this lesson prints both columns.
  CellIndex cell_for(double x, double y) const {
    (void)x;
    (void)y;
    return CellIndex{};
  }

  double log_odds(int x, int y) const {
    if (!inside(x, y)) return 0.0;
    return cells_[index_of(x, y)];
  }

  double probability(int x, int y) const {
    const double l = log_odds(x, y);
    return 1.0 - 1.0 / (1.0 + std::exp(l));
  }

  // TODO 2: the map's answer for one cell.
  //
  //   occupied  when the log odds are at or above  kDecided
  //   free      when they are at or below         -kDecided
  //   unknown   in between, which includes a cell nobody has observed
  //
  // The third answer is the one that matters. A map with two answers has to
  // call unlooked-at ground something, and whichever it picks is wrong.
  Cell classify(int x, int y) const {
    (void)x;
    (void)y;
    return Cell::unknown;
  }

  // TODO 3: one observation of one cell.
  //
  // Add kHit or kMiss to the cell, then clamp the result to plus and minus
  // kClamp. Ignore anything outside the grid.
  //
  // The clamp is the whole defence against a map that cannot be corrected.
  // Without it, evidence accumulates without limit and a cell seen occupied a
  // few hundred times needs a few hundred contrary readings to change its mind,
  // by which time the robot has driven into it. With it, five will do, however
  // long the cell has been sure.
  void observe(int x, int y, bool occupied) {
    (void)x;
    (void)y;
    (void)occupied;
  }

  // One range reading: everything the beam passed through is free, and where it
  // stopped is occupied.
  //
  // `hit` is whether the beam stopped on something rather than running out to
  // the sensor's maximum. This part is written for you, because Bresenham is
  // not the lesson; what it does with the endpoint is.
  void integrate_ray(double from_x, double from_y, double to_x, double to_y, bool hit) {
    const CellIndex start = cell_for(from_x, from_y);
    const CellIndex end = cell_for(to_x, to_y);

    int x = start.x, y = start.y;
    const int dx = std::abs(end.x - x), dy = -std::abs(end.y - y);
    const int step_x = x < end.x ? 1 : -1, step_y = y < end.y ? 1 : -1;
    int error = dx + dy;

    while (true) {
      if (x == end.x && y == end.y) break;
      observe(x, y, false);

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

#endif  // LESSON_SOLUTION_HPP
