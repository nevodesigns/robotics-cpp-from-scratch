#include <rc/test/rc_test.hpp>
#include <rc/nav/plan.hpp>

#include <cmath>
#include <iomanip>
#include <iostream>
#include <vector>

#include "solution.hpp"

namespace {

void mark(OccupancyGrid& grid, int x, int y, bool occupied) {
  grid.observe(x, y, occupied);
  grid.observe(x, y, occupied);   // two observations to be decided either way
}

// A room 60 by 40 cells, all seen, with a wall at x = 20 open above y = 28 and a wall at x = 40 open below y = 12.
OccupancyGrid room() {
  OccupancyGrid grid(60, 40, 0.1, 0.0, 0.0);
  for (int y = 0; y < 40; ++y)
    for (int x = 0; x < 60; ++x) {
      const bool wall = (x == 20 && y <= 28) || (x == 40 && y >= 12);
      mark(grid, x, y, wall);
    }
  return grid;
}

// The length of a path through its cells, the way the planner counts it.
double length_of(const std::vector<GridPoint>& p) { return rc::nav::path_length(p); }

// A check of a straight segment that does not use line_is_clear: sample it every tenth of a cell and ask which cell
// each sample is in. A sample inside a wall, or a segment that passes between two touching walls, is blocked.
bool segment_blocked(const OccupancyGrid& grid, GridPoint a, GridPoint b) {
  const double length = std::hypot(static_cast<double>(b.x - a.x), static_cast<double>(b.y - a.y));
  const int samples = static_cast<int>(length * 10.0) + 1;
  for (int i = 0; i <= samples; ++i) {
    const double f = static_cast<double>(i) / samples;
    const double x = a.x + (b.x - a.x) * f + 0.5;   // the centre of a cell is at its index plus a half
    const double y = a.y + (b.y - a.y) * f + 0.5;
    if (grid.classify(static_cast<int>(std::floor(x)), static_cast<int>(std::floor(y))) != Cell::free) return true;
  }
  return false;
}

}  // namespace

RC_TEST("a line over free cells is clear, and one that touches a wall or an unseen cell is not") {
  OccupancyGrid grid = room();
  RC_CHECK(line_is_clear(grid, GridPoint{2, 2}, GridPoint{18, 2}));
  RC_CHECK(line_is_clear(grid, GridPoint{2, 2}, GridPoint{18, 15}));
  RC_CHECK(!line_is_clear(grid, GridPoint{2, 2}, GridPoint{30, 2}));        // through the wall at x = 20
  RC_CHECK(!line_is_clear(grid, GridPoint{20, 5}, GridPoint{25, 5}));       // starts inside a wall
  RC_CHECK(line_is_clear(grid, GridPoint{5, 5}, GridPoint{5, 5}));          // a point is clear if its cell is

  OccupancyGrid unseen(20, 20, 0.1, 0.0, 0.0);
  for (int x = 0; x < 20; ++x) mark(unseen, x, 0, false);
  RC_CHECK(line_is_clear(unseen, GridPoint{0, 0}, GridPoint{19, 0}));
  RC_CHECK(!line_is_clear(unseen, GridPoint{0, 0}, GridPoint{19, 5}));      // leaves the cells that were seen
  RC_CHECK(!line_is_clear(grid, GridPoint{-3, 2}, GridPoint{5, 2}));        // outside the grid
}

RC_TEST("a line may not slip between two walls that only touch at their corners") {
  OccupancyGrid grid(10, 10, 0.1, 0.0, 0.0);
  for (int y = 0; y < 10; ++y)
    for (int x = 0; x < 10; ++x) mark(grid, x, y, false);
  // Two walls touching at a corner: (4, 5) and (5, 4). The diagonal from (3, 3) to (6, 6) passes between them.
  mark(grid, 4, 5, true);
  mark(grid, 5, 4, true);
  const bool clear = line_is_clear(grid, GridPoint{3, 3}, GridPoint{6, 6});
  std::cout << "  walls at (4,5) and (5,4), diagonal from (3,3) to (6,6): cells on the line are free, and the line is "
            << (clear ? "called clear" : "called blocked") << "\n\n";
  RC_CHECK(!clear);
  RC_CHECK(line_is_clear(grid, GridPoint{0, 0}, GridPoint{3, 3}));          // open ground is still open
  RC_CHECK(line_is_clear(grid, GridPoint{0, 9}, GridPoint{9, 9}));
}

RC_TEST("a planned path is shortened to a few corners, and every piece of it is clear") {
  OccupancyGrid grid = room();
  rc::nav::PlanOptions options;
  const rc::nav::Plan plan = rc::nav::plan_path(grid, GridPoint{3, 3}, GridPoint{56, 36}, options);
  RC_REQUIRE(plan.found);
  const std::vector<GridPoint> tight = shorten(grid, plan.cells);

  std::cout << "  planned path from (3,3) to (56,36)\n"
            << "                 points   length (cells)   turns over 10 degrees\n"
            << "  as planned     " << std::setw(6) << plan.cells.size() << "   " << std::fixed << std::setprecision(2) << std::setw(14)
            << length_of(plan.cells) << "   " << std::setw(8) << count_turns(plan.cells, 0.1745) << "\n"
            << "  shortened      " << std::setw(6) << tight.size() << "   " << std::setw(14) << length_of(tight) << "   " << std::setw(8)
            << count_turns(tight, 0.1745) << "\n\n";

  RC_REQUIRE(tight.size() >= 2);
  RC_CHECK(tight.size() < plan.cells.size() / 3);
  RC_CHECK(length_of(tight) <= length_of(plan.cells) + 1e-9);
  RC_CHECK(tight.front().x == 3 && tight.front().y == 3);
  RC_CHECK(tight.back().x == 56 && tight.back().y == 36);
  bool all_clear = true;
  for (std::size_t i = 1; i < tight.size(); ++i)
    if (segment_blocked(grid, tight[i - 1], tight[i])) all_clear = false;
  RC_CHECK(all_clear);
  RC_CHECK(count_turns(tight, 0.1745) < count_turns(plan.cells, 0.1745));
}

RC_TEST("shortening never makes a path longer, never loses its ends, and leaves a path that is already straight alone") {
  OccupancyGrid grid = room();
  const std::vector<GridPoint> none;
  RC_CHECK(shorten(grid, none).empty());

  std::vector<GridPoint> one = {GridPoint{5, 5}};
  RC_CHECK(shorten(grid, one).size() == 1);

  std::vector<GridPoint> straight;
  for (int x = 2; x <= 15; ++x) straight.push_back(GridPoint{x, 3});
  const std::vector<GridPoint> s = shorten(grid, straight);
  RC_REQUIRE(s.size() == 2);
  RC_CHECK(s.front().x == 2 && s.back().x == 15);

  // Across many planned paths, in the same room.
  int checked = 0;
  int longer = 0;
  int ends_lost = 0;
  for (int sx = 2; sx < 18; sx += 4)
    for (int gx = 44; gx < 58; gx += 4) {
      const rc::nav::Plan p = rc::nav::plan_path(grid, GridPoint{sx, 2}, GridPoint{gx, 37}, rc::nav::PlanOptions());
      if (!p.found) continue;
      const std::vector<GridPoint> t = shorten(grid, p.cells);
      ++checked;
      if (length_of(t) > length_of(p.cells) + 1e-9) ++longer;
      if (t.empty() || t.front().x != sx || t.back().x != gx) ++ends_lost;
    }
  std::cout << "  " << checked << " planned paths shortened: " << longer << " got longer, " << ends_lost << " lost an end\n\n";
  RC_CHECK(checked >= 12);
  RC_CHECK(longer == 0);
  RC_CHECK(ends_lost == 0);
}

RC_TEST("turns are counted by how much the heading changes, the short way round") {
  const std::vector<GridPoint> zigzag = {{0, 0}, {1, 0}, {2, 0}, {3, 1}, {4, 2}, {4, 3}, {4, 4}};
  RC_CHECK(count_turns(zigzag, 0.1) == 2);                      // east to north east, then north east to north
  RC_CHECK(count_turns(zigzag, 0.9) == 0);                      // each turn is 45 degrees, under 0.9 rad
  const std::vector<GridPoint> reverse = {{0, 0}, {1, 0}, {0, 0}};
  RC_CHECK(count_turns(reverse, 3.0) == 1);                     // 180 degrees is a turn
  const std::vector<GridPoint> wrap = {{0, 0}, {-1, 0}, {-2, -1}};   // west, then south west: 45 degrees, not 315
  RC_CHECK(count_turns(wrap, 1.0) == 0);
  RC_CHECK(count_turns(wrap, 0.5) == 1);
  const std::vector<GridPoint> repeated = {{0, 0}, {0, 0}, {1, 0}, {1, 0}, {2, 0}};
  RC_CHECK(count_turns(repeated, 0.1) == 0);
  RC_CHECK(count_turns(std::vector<GridPoint>(), 0.1) == 0);
  RC_CHECK(count_turns(zigzag, 0.1) == 2);
}
