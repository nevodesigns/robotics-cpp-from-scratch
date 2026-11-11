#include <rc/test/rc_test.hpp>

#include <cmath>
#include <iomanip>
#include <iostream>
#include <queue>
#include <vector>

#include "solution.hpp"

namespace {

void mark(OccupancyGrid& grid, int x, int y, bool occupied) {
  grid.observe(x, y, occupied);
  grid.observe(x, y, occupied);
}

// A map 30 by 20. The left two thirds, x below 20, has been seen: free, with a wall at x = 10 for y up to 14.
// The right third has not. One cell at (3, 3) was missed inside the seen part.
OccupancyGrid half_known(bool speck) {
  OccupancyGrid grid(30, 20, 0.1, 0.0, 0.0);
  for (int y = 0; y < 20; ++y)
    for (int x = 0; x < 20; ++x) {
      if (speck && x == 3 && y == 3) continue;
      mark(grid, x, y, x == 10 && y <= 14);
    }
  return grid;
}

// The distance in steps over free cells, four neighbours, worked out here and not by the code under test. -1 if there is no way.
int steps_between(const OccupancyGrid& grid, GridPoint from, GridPoint to) {
  std::vector<int> dist(static_cast<std::size_t>(grid.width()) * grid.height(), -1);
  std::queue<GridPoint> q;
  q.push(from);
  dist[static_cast<std::size_t>(from.y) * grid.width() + from.x] = 0;
  const int dx[4] = {1, -1, 0, 0};
  const int dy[4] = {0, 0, 1, -1};
  while (!q.empty()) {
    const GridPoint p = q.front();
    q.pop();
    if (p.x == to.x && p.y == to.y) return dist[static_cast<std::size_t>(p.y) * grid.width() + p.x];
    for (int i = 0; i < 4; ++i) {
      const int nx = p.x + dx[i];
      const int ny = p.y + dy[i];
      if (!grid.inside(nx, ny) || grid.classify(nx, ny) != Cell::free) continue;
      const std::size_t k = static_cast<std::size_t>(ny) * grid.width() + nx;
      if (dist[k] >= 0) continue;
      dist[k] = dist[static_cast<std::size_t>(p.y) * grid.width() + p.x] + 1;
      q.push(GridPoint{nx, ny});
    }
  }
  return -1;
}

}  // namespace

RC_TEST("a frontier is a free cell next to one nobody has seen, and a wall or an unseen cell is not") {
  OccupancyGrid grid = half_known(false);
  RC_CHECK(is_frontier(grid, 19, 5));        // free, and (20, 5) is unseen
  RC_CHECK(!is_frontier(grid, 18, 5));       // free, every neighbour seen
  RC_CHECK(!is_frontier(grid, 20, 5));       // unseen itself
  RC_CHECK(!is_frontier(grid, 10, 5));       // a wall
  RC_CHECK(!is_frontier(grid, -1, 5));       // outside
  RC_CHECK(!is_frontier(grid, 5, 25));
  const std::vector<GridPoint> f = find_frontiers(grid);
  std::cout << "  map 30 by 20, left two thirds seen: " << f.size() << " frontier cells\n\n";
  RC_CHECK(f.size() == 20);                  // the whole column x = 19
  bool column = !f.empty();
  for (const GridPoint& p : f)
    if (p.x != 19) column = false;
  RC_CHECK(column);
}

RC_TEST("touching frontier cells are one opening, and a single missed cell makes a small opening of its own") {
  OccupancyGrid grid = half_known(true);
  const std::vector<std::vector<GridPoint>> clusters = cluster_frontiers(grid);
  std::cout << "  with one cell missed at (3,3): " << clusters.size() << " openings, of sizes";
  std::size_t small = 0;
  std::size_t large = 0;
  for (const auto& c : clusters) {
    std::cout << " " << c.size();
    if (c.size() < 10) small = c.size();
    else large = c.size();
  }
  std::cout << "\n\n";
  RC_CHECK(clusters.size() == 2);
  if (clusters.size() != 2) return;   // not RC_REQUIRE, which would skip the destructors
  RC_CHECK(small == 4);                      // the four cells around the speck
  RC_CHECK(large == 20);
}

RC_TEST("the nearest opening is chosen by the way the rover would drive, not by the straight line through the wall") {
  OccupancyGrid grid = half_known(false);
  const GridPoint start{5, 5};
  GridPoint goal{-1, -1};
  const Explore result = choose_frontier(grid, start, 5, goal);
  RC_CHECK(result == Explore::found);
  if (result != Explore::found) return;

  // The frontier cell nearest the start in a straight line, for comparison.
  GridPoint straight = goal;
  double best = 1e9;
  for (const GridPoint& p : find_frontiers(grid)) {
    const double d = std::hypot(static_cast<double>(p.x - start.x), static_cast<double>(p.y - start.y));
    if (d < best) {
      best = d;
      straight = p;
    }
  }
  const int chosen_steps = steps_between(grid, start, goal);
  const int straight_steps = steps_between(grid, start, straight);

  int fewest = 1 << 30;
  for (const GridPoint& p : find_frontiers(grid)) {
    const int s = steps_between(grid, start, p);
    if (s >= 0 && s < fewest) fewest = s;
  }
  std::cout << "  from (5,5), with a wall between it and the unseen part\n"
            << "  nearest in a straight line: (" << straight.x << "," << straight.y << "), " << straight_steps << " steps to drive\n"
            << "  chosen:                     (" << goal.x << "," << goal.y << "), " << chosen_steps << " steps to drive\n"
            << "  fewest steps to any frontier cell: " << fewest << "\n\n";
  RC_CHECK(chosen_steps == fewest);
  RC_CHECK(straight_steps > chosen_steps);
}

RC_TEST("a single missed cell is ignored by a minimum size, and sent to by none") {
  OccupancyGrid grid = half_known(true);
  GridPoint goal{-1, -1};
  const Explore first = choose_frontier(grid, GridPoint{5, 5}, 5, goal);
  RC_CHECK(first == Explore::found);
  if (first != Explore::found) return;
  RC_CHECK(goal.x == 19 || goal.x > 10);     // the real opening, past the wall

  GridPoint any{-1, -1};
  const Explore second = choose_frontier(grid, GridPoint{4, 4}, 1, any);
  RC_CHECK(second == Explore::found);
  if (second != Explore::found) return;
  std::cout << "  starting at (4,4), next to the missed cell: minimum size 5 goes to (" << goal.x << "," << goal.y
            << "), minimum size 1 goes to (" << any.x << "," << any.y << ")\n\n";
  RC_CHECK(std::abs(any.x - 3) + std::abs(any.y - 3) <= 2);     // right beside the speck
}

RC_TEST("finished and stuck are different answers: nothing left to see, and something left that cannot be reached") {
  // Finished: everything seen.
  OccupancyGrid done(10, 10, 0.1, 0.0, 0.0);
  for (int y = 0; y < 10; ++y)
    for (int x = 0; x < 10; ++x) mark(done, x, y, false);
  GridPoint goal{-1, -1};
  const Explore a = choose_frontier(done, GridPoint{1, 1}, 1, goal);

  // Stuck: the rover is in a sealed room of seen cells. A sliver on the far side of the wall was seen through a window, so there is a
  // frontier, and it cannot be reached from where the rover is.
  OccupancyGrid sealed(20, 10, 0.1, 0.0, 0.0);
  for (int y = 0; y < 10; ++y)
    for (int x = 0; x <= 12; ++x) mark(sealed, x, y, x == 10);
  const Explore b = choose_frontier(sealed, GridPoint{2, 5}, 1, goal);

  // The rover standing somewhere it cannot be: inside a wall.
  const Explore c = choose_frontier(sealed, GridPoint{10, 5}, 1, goal);
  std::cout << "  everything seen: " << (a == Explore::finished ? "finished" : "other") << "; a frontier on the far side of a wall: "
            << (b == Explore::unreachable ? "unreachable" : "other") << "; start inside a wall: "
            << (c == Explore::unreachable ? "unreachable" : "other") << "\n\n";
  RC_CHECK(a == Explore::finished);
  RC_CHECK(b == Explore::unreachable);
  RC_CHECK(c == Explore::unreachable);
  RC_CHECK(choose_frontier(half_known(false), GridPoint{5, 5}, 5, goal) == Explore::found);
}
