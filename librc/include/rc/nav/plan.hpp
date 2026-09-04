// rc/nav/plan.hpp
//
// A search over the map, from lesson 16-06.
//
// The search itself is a hundred lines of A* and is not where the difficulty
// is. Three numbers decide whether its answer means anything, and each of them
// looks like a detail:
//
// The diagonal step costs sqrt(2). Charging one for it made the search report
// 59.0 for a path 81.4 long, and the path it chose was 11 percent longer than
// the one it was looking for.
//
// The heuristic must not overestimate. Manhattan distance on a grid you may
// cross diagonally overestimates by 0.586 per shared step: measured over a
// hundred cluttered maps it expanded a fifth fewer cells and returned a
// suboptimal path on three maps in ten, by up to 3.45 percent.
//
// Unknown ground has no safe default. Blocked, expensive or free is a decision
// about the robot rather than about the code, and with expensive the multiplier
// is where the decision actually lives.

#ifndef RC_NAV_PLAN
#define RC_NAV_PLAN

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <functional>
#include <limits>
#include <queue>
#include <utility>
#include <vector>

#include <rc/nav/grid.hpp>

namespace rc {
namespace nav {

struct GridPoint {
  int x = 0;
  int y = 0;
};

// What the planner should do about ground nobody has looked at.
//
// There is no safe default here, only a decision somebody has to make out loud.
// Blocked is right for a robot that must not surprise anyone. Expensive is right
// for one that may explore when the known route is much worse. Free is right
// almost nowhere, and it is what a grid of booleans gives you by accident.
enum class UnknownIs {
  blocked,
  expensive,
  free,
};

// Which guess the search uses about the distance still to go.
//
// A heuristic that never overestimates makes A* faster without changing what it
// finds. One that overestimates makes it faster still and no longer optimal,
// and this is a parameter here so the difference can be measured rather than
// asserted.
enum class Guess {
  none,        // no guess at all, which is Dijkstra
  octile,      // the true distance over a grid you may cross diagonally
  manhattan,   // the two axes counted separately, which overestimates
};

struct PlanOptions {
  UnknownIs unknown = UnknownIs::blocked;
  double unknown_multiplier = 4.0;   // when unknown is merely expensive
  bool allow_diagonal = true;
  Guess guess = Guess::octile;
};

struct Plan {
  bool found = false;
  double cost = 0.0;            // what the search added up
  int expanded = 0;             // how many cells it had to look at
  std::vector<GridPoint> cells;
};

// The straight line distance over a grid where you may move diagonally.
//
// Not the Manhattan distance. A diagonal step covers one cell of x and one of
// y at once, so counting them separately overestimates by 0.586 for every cell
// they share, which makes A* faster and its answer no longer optimal.
inline double octile_distance(int x0, int y0, int x1, int y1) {
  const double dx = std::fabs(static_cast<double>(x1 - x0));
  const double dy = std::fabs(static_cast<double>(y1 - y0));
  const double shared = std::min(dx, dy);
  return (dx + dy) + (std::sqrt(2.0) - 2.0) * shared;
}

// What one step onto a neighbouring cell costs, or a negative number if it
// cannot be taken.
//
// The diagonal costs the square root of two, not one. Charging one for it makes
// the number the planner reports stop meaning anything: measured on a map with
// two walls, the search reported 59.0 for a path 81.4 long.
inline double step_cost(const OccupancyGrid& grid, int from_x, int from_y, int to_x,
                        int to_y, const PlanOptions& options) {
  if (!grid.inside(to_x, to_y)) return -1.0;

  const Cell cell = grid.classify(to_x, to_y);
  if (cell == Cell::occupied) return -1.0;

  double multiplier = 1.0;
  if (cell == Cell::unknown) {
    if (options.unknown == UnknownIs::blocked) return -1.0;
    if (options.unknown == UnknownIs::expensive) multiplier = options.unknown_multiplier;
  }

  const int dx = to_x - from_x, dy = to_y - from_y;
  const bool diagonal = dx != 0 && dy != 0;
  if (diagonal && !options.allow_diagonal) return -1.0;

  return (diagonal ? std::sqrt(2.0) : 1.0) * multiplier;
}

// How long the path actually is, in cells.
//
// This is not the search's cost and the two are worth comparing. The cost is
// whatever the step function charged; the length is what the robot will drive.
// They agree only when the step function was telling the truth about geometry.
inline double path_length(const std::vector<GridPoint>& cells) {
  double total = 0.0;
  for (std::size_t i = 1; i < cells.size(); ++i)
    total += std::hypot(static_cast<double>(cells[i].x - cells[i - 1].x),
                        static_cast<double>(cells[i].y - cells[i - 1].y));
  return total;
}

inline double guess_remaining(Guess guess, int x, int y, GridPoint goal) {
  switch (guess) {
    case Guess::none: return 0.0;
    case Guess::octile: return octile_distance(x, y, goal.x, goal.y);
    case Guess::manhattan:
      return std::fabs(static_cast<double>(goal.x - x)) +
             std::fabs(static_cast<double>(goal.y - y));
  }
  return 0.0;
}

// A* over the grid.
//
// The search itself is short and unremarkable. Everything that decides whether
// its answer is worth anything is in the two functions above.
inline Plan plan_path(const OccupancyGrid& grid, GridPoint start, GridPoint goal,
                      const PlanOptions& options) {
  Plan plan;
  if (!grid.inside(start.x, start.y) || !grid.inside(goal.x, goal.y)) return plan;

  const int width = grid.width();
  const std::size_t count = static_cast<std::size_t>(width) *
                            static_cast<std::size_t>(grid.height());
  const double infinity = std::numeric_limits<double>::infinity();

  std::vector<double> best(count, infinity);
  std::vector<int> came_from(count, -1);
  std::vector<bool> settled(count, false);

  const auto index_of = [width](int x, int y) {
    return static_cast<std::size_t>(y) * static_cast<std::size_t>(width) +
           static_cast<std::size_t>(x);
  };

  using Entry = std::pair<double, std::size_t>;
  std::priority_queue<Entry, std::vector<Entry>, std::greater<Entry>> open;

  const std::size_t start_index = index_of(start.x, start.y);
  const std::size_t goal_index = index_of(goal.x, goal.y);
  best[start_index] = 0.0;
  open.push({guess_remaining(options.guess, start.x, start.y, goal), start_index});

  while (!open.empty()) {
    const std::size_t current = open.top().second;
    open.pop();

    // The lazy deletion a std::priority_queue forces on you: an entry may be a
    // stale copy of a cell already settled at a lower cost. Skipping it here is
    // cheaper than finding and updating it in the queue.
    if (settled[current]) continue;
    settled[current] = true;
    ++plan.expanded;
    if (current == goal_index) break;

    const int cx = static_cast<int>(current % static_cast<std::size_t>(width));
    const int cy = static_cast<int>(current / static_cast<std::size_t>(width));

    for (int dy = -1; dy <= 1; ++dy) {
      for (int dx = -1; dx <= 1; ++dx) {
        if (dx == 0 && dy == 0) continue;
        const int nx = cx + dx, ny = cy + dy;
        const double step = step_cost(grid, cx, cy, nx, ny, options);
        if (step < 0.0) continue;

        const std::size_t next = index_of(nx, ny);
        const double candidate = best[current] + step;
        if (candidate + 1e-12 < best[next]) {
          best[next] = candidate;
          came_from[next] = static_cast<int>(current);
          open.push({candidate + guess_remaining(options.guess, nx, ny, goal), next});
        }
      }
    }
  }

  if (best[goal_index] == infinity) return plan;

  plan.found = true;
  plan.cost = best[goal_index];
  for (int at = static_cast<int>(goal_index); at != -1; at = came_from[at])
    plan.cells.push_back({static_cast<int>(at % width), static_cast<int>(at / width)});
  std::reverse(plan.cells.begin(), plan.cells.end());
  return plan;
}

}  // namespace nav
}  // namespace rc

#endif  // RC_NAV_PLAN
