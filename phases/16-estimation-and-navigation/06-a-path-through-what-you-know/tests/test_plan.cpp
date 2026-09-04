#include <rc/test/rc_test.hpp>

#include <rc/nav/grid.hpp>

#include <cmath>
#include <iomanip>
#include <iostream>
#include <vector>

#include "solution.hpp"

namespace {

constexpr int kWidth = 60;
constexpr int kHeight = 40;

// Enough observations to make the map commit to an answer.
void make_free(OccupancyGrid& grid, int x, int y) {
  grid.observe(x, y, false);
  grid.observe(x, y, false);
}

void make_occupied(OccupancyGrid& grid, int x, int y) {
  grid.observe(x, y, true);
  grid.observe(x, y, true);
}

// A room the robot has surveyed, with two walls it has to weave between: one
// with a gap at the top, one with a gap at the bottom.
OccupancyGrid surveyed_room() {
  OccupancyGrid grid(kWidth, kHeight, 0.10, 0.0, 0.0);
  for (int y = 0; y < kHeight; ++y)
    for (int x = 0; x < kWidth; ++x) make_free(grid, x, y);
  for (int y = 0; y < 30; ++y) make_occupied(grid, 25, y);
  for (int y = 12; y < kHeight; ++y) make_occupied(grid, 40, y);
  return grid;
}

// The same room, with a scattering of obstacles from a repeatable sequence.
OccupancyGrid cluttered_room(unsigned seed, double density) {
  OccupancyGrid grid(kWidth, kHeight, 0.10, 0.0, 0.0);
  unsigned state = seed * 1103515245u + 12345u;
  for (int y = 0; y < kHeight; ++y) {
    for (int x = 0; x < kWidth; ++x) {
      state = state * 1103515245u + 12345u;
      const double unit = static_cast<double>((state >> 16) & 0x7fffu) / 32768.0;
      if (unit < density) make_occupied(grid, x, y);
      else make_free(grid, x, y);
    }
  }
  return grid;
}

}  // namespace

RC_TEST("the heuristic knows a diagonal step covers two cells at once") {
  // Straight along a row: the two agree.
  RC_CHECK_NEAR(octile_distance(0, 0, 10, 0), 10.0, 1e-12);
  RC_CHECK_NEAR(octile_distance(0, 0, 0, 7), 7.0, 1e-12);

  // Perfectly diagonal: ten steps of root two, not twenty steps of one.
  RC_CHECK_NEAR(octile_distance(0, 0, 10, 10), 10.0 * std::sqrt(2.0), 1e-12);

  // Mixed: the shared part goes diagonally, the rest straight.
  RC_CHECK_NEAR(octile_distance(0, 0, 10, 3), 7.0 + 3.0 * std::sqrt(2.0), 1e-12);

  // Never longer than counting the two axes separately, which is what makes it
  // admissible and Manhattan not.
  for (int dx = 0; dx <= 12; ++dx)
    for (int dy = 0; dy <= 12; ++dy)
      RC_CHECK(octile_distance(0, 0, dx, dy) <= dx + dy + 1e-12);

  // And symmetric in every direction.
  RC_CHECK_NEAR(octile_distance(5, 5, 0, 2), octile_distance(0, 2, 5, 5), 1e-12);
  RC_CHECK_NEAR(octile_distance(0, 0, -4, -4), octile_distance(0, 0, 4, 4), 1e-12);
}

RC_TEST("a step onto a cell costs what the geometry says, or cannot be taken") {
  OccupancyGrid grid(10, 10, 0.10, 0.0, 0.0);
  for (int y = 0; y < 10; ++y)
    for (int x = 0; x < 10; ++x) make_free(grid, x, y);
  make_occupied(grid, 5, 5);

  PlanOptions options;
  RC_CHECK_NEAR(step_cost(grid, 1, 1, 2, 1, options), 1.0, 1e-12);
  RC_CHECK_NEAR(step_cost(grid, 1, 1, 2, 2, options), std::sqrt(2.0), 1e-12);

  // Occupied and off the map are both refused.
  RC_CHECK(step_cost(grid, 4, 5, 5, 5, options) < 0.0);
  RC_CHECK(step_cost(grid, 0, 0, -1, 0, options) < 0.0);

  // Unknown is whatever the caller decided, and there is no default worth
  // having: this map has never been observed at all.
  OccupancyGrid unseen(10, 10, 0.10, 0.0, 0.0);
  PlanOptions blocked;
  blocked.unknown = UnknownIs::blocked;
  RC_CHECK(step_cost(unseen, 1, 1, 2, 1, blocked) < 0.0);

  PlanOptions expensive;
  expensive.unknown = UnknownIs::expensive;
  expensive.unknown_multiplier = 4.0;
  RC_CHECK_NEAR(step_cost(unseen, 1, 1, 2, 1, expensive), 4.0, 1e-12);

  PlanOptions permissive;
  permissive.unknown = UnknownIs::free;
  RC_CHECK_NEAR(step_cost(unseen, 1, 1, 2, 1, permissive), 1.0, 1e-12);

  // Turning diagonals off costs geometry and buys a path a differential drive
  // can follow without a corner in every cell.
  PlanOptions straight;
  straight.allow_diagonal = false;
  RC_CHECK(step_cost(grid, 1, 1, 2, 2, straight) < 0.0);
  RC_CHECK_NEAR(step_cost(grid, 1, 1, 2, 1, straight), 1.0, 1e-12);
}

RC_TEST("the planner's number is not the path's length unless you make it so") {
  const OccupancyGrid grid = surveyed_room();
  const GridPoint start{2, 20}, goal{57, 20};

  PlanOptions options;
  const Plan plan = plan_path(grid, start, goal, options);
  RC_REQUIRE(plan.found);

  std::cout << "\n    a room with two walls, weaving between the gaps\n\n";
  std::cout << "    " << std::left << std::setw(26) << "cost the search added up"
            << std::right << std::fixed << std::setprecision(4) << plan.cost << "\n";
  std::cout << "    " << std::left << std::setw(26) << "length of the path"
            << std::right << path_length(plan.cells) << "\n";
  std::cout << "    " << std::left << std::setw(26) << "cells expanded" << std::right
            << plan.expanded << "\n";

  // With the diagonal costing root two, the two numbers are the same number.
  RC_CHECK_NEAR(plan.cost, path_length(plan.cells), 1e-9);

  // The path starts where it was asked to and ends where it was sent, and every
  // step is to a neighbouring cell.
  RC_CHECK_EQ(plan.cells.front().x, start.x);
  RC_CHECK_EQ(plan.cells.front().y, start.y);
  RC_CHECK_EQ(plan.cells.back().x, goal.x);
  RC_CHECK_EQ(plan.cells.back().y, goal.y);
  for (std::size_t i = 1; i < plan.cells.size(); ++i) {
    RC_CHECK(std::abs(plan.cells[i].x - plan.cells[i - 1].x) <= 1);
    RC_CHECK(std::abs(plan.cells[i].y - plan.cells[i - 1].y) <= 1);
  }

  // And it does not walk through a wall.
  for (const GridPoint& cell : plan.cells)
    RC_CHECK(grid.classify(cell.x, cell.y) != rc::nav::Cell::occupied);
}

RC_TEST("a diagonal that costs one") {
  const OccupancyGrid grid = surveyed_room();
  const GridPoint start{2, 20}, goal{57, 20};

  const Plan honest = plan_path(grid, start, goal, PlanOptions{});

  // The same search with every step charged at one, which is what a grid
  // planner does when nobody thought about the diagonal.
  double unit_cost = 0.0, unit_length = 0.0;
  {
    const int width = grid.width();
    const std::size_t count = static_cast<std::size_t>(width) *
                              static_cast<std::size_t>(grid.height());
    std::vector<double> best(count, 1e300);
    std::vector<int> came(count, -1);
    std::vector<bool> settled(count, false);
    const auto id = [width](int x, int y) {
      return static_cast<std::size_t>(y) * static_cast<std::size_t>(width) +
             static_cast<std::size_t>(x);
    };
    std::vector<std::pair<double, std::size_t>> open;
    best[id(start.x, start.y)] = 0.0;
    open.push_back({0.0, id(start.x, start.y)});
    std::make_heap(open.begin(), open.end(), std::greater<std::pair<double, std::size_t>>());

    while (!open.empty()) {
      std::pop_heap(open.begin(), open.end(), std::greater<std::pair<double, std::size_t>>());
      const std::size_t current = open.back().second;
      open.pop_back();
      if (settled[current]) continue;
      settled[current] = true;
      const int cx = static_cast<int>(current % static_cast<std::size_t>(width));
      const int cy = static_cast<int>(current / static_cast<std::size_t>(width));
      if (cx == goal.x && cy == goal.y) break;
      for (int dy = -1; dy <= 1; ++dy)
        for (int dx = -1; dx <= 1; ++dx) {
          if (dx == 0 && dy == 0) continue;
          const int nx = cx + dx, ny = cy + dy;
          if (!grid.inside(nx, ny)) continue;
          if (grid.classify(nx, ny) != rc::nav::Cell::free) continue;
          const std::size_t next = id(nx, ny);
          if (best[current] + 1.0 + 1e-12 < best[next]) {
            best[next] = best[current] + 1.0;
            came[next] = static_cast<int>(current);
            open.push_back({best[next], next});
            std::push_heap(open.begin(), open.end(),
                           std::greater<std::pair<double, std::size_t>>());
          }
        }
    }

    std::vector<GridPoint> cells;
    for (int at = static_cast<int>(id(goal.x, goal.y)); at != -1; at = came[at])
      cells.push_back({at % width, at / width});
    std::reverse(cells.begin(), cells.end());
    unit_cost = best[id(goal.x, goal.y)];
    unit_length = path_length(cells);
  }

  std::cout << "\n    " << std::right << std::setw(20) << "diagonal step"
            << std::setw(12) << "reported" << std::setw(14) << "true length"
            << std::setw(14) << "difference" << "\n";
  std::cout << "    " << std::right << std::setw(20) << "1.0" << std::fixed
            << std::setprecision(4) << std::setw(12) << unit_cost << std::setw(14)
            << unit_length << std::setprecision(1) << std::setw(13)
            << (unit_length / unit_cost - 1.0) * 100.0 << "%" << "\n";
  std::cout << "    " << std::right << std::setw(20) << "sqrt(2)" << std::fixed
            << std::setprecision(4) << std::setw(12) << honest.cost << std::setw(14)
            << path_length(honest.cells) << std::setprecision(1) << std::setw(13)
            << 0.0 << "%" << "\n";

  std::cout << "\n    the cheap answer is not merely mislabelled: the path it\n";
  std::cout << "    chose is " << std::setprecision(1)
            << (unit_length / path_length(honest.cells) - 1.0) * 100.0
            << "% longer than the one it was looking for\n";

  // The reported cost understates the distance by more than a third.
  RC_CHECK(unit_length > unit_cost * 1.3);

  // And the path itself is genuinely worse, not just described wrongly.
  RC_CHECK(unit_length > path_length(honest.cells) * 1.05);

  // While the honest one reports exactly what it will drive.
  RC_CHECK_NEAR(honest.cost, path_length(honest.cells), 1e-9);
}

RC_TEST("what unknown ground is allowed to cost") {
  // A map surveyed only along the bottom. Everything above row 8 is unseen.
  OccupancyGrid grid(kWidth, kHeight, 0.10, 0.0, 0.0);
  for (int y = 0; y < 8; ++y)
    for (int x = 0; x < kWidth; ++x) make_free(grid, x, y);
  for (int x = 20; x < 45; ++x)
    for (int y = 0; y < 6; ++y) make_occupied(grid, x, y);

  const GridPoint start{2, 3}, goal{57, 3};

  PlanOptions blocked;
  blocked.unknown = UnknownIs::blocked;
  const Plan careful = plan_path(grid, start, goal, blocked);

  PlanOptions permissive;
  permissive.unknown = UnknownIs::free;
  const Plan bold = plan_path(grid, start, goal, permissive);

  PlanOptions costly;
  costly.unknown = UnknownIs::expensive;
  costly.unknown_multiplier = 4.0;
  const Plan cautious = plan_path(grid, start, goal, costly);

  PlanOptions slightly;
  slightly.unknown = UnknownIs::expensive;
  slightly.unknown_multiplier = 1.05;
  const Plan tempted = plan_path(grid, start, goal, slightly);

  const auto unseen_cells = [&](const Plan& plan) {
    int count = 0;
    for (const GridPoint& cell : plan.cells)
      if (grid.classify(cell.x, cell.y) == rc::nav::Cell::unknown) ++count;
    return count;
  };

  std::cout << "\n    a corridor surveyed only along the bottom, an obstacle\n";
  std::cout << "    across it, and open ground nobody has looked at above\n\n";
  std::cout << "    " << std::left << std::setw(16) << "unknown is" << std::right
            << std::setw(10) << "found" << std::setw(12) << "length"
            << std::setw(18) << "unseen cells" << "\n";
  const char* names[] = {"blocked", "expensive x4", "expensive x1.05", "free"};
  const Plan* plans[] = {&careful, &cautious, &tempted, &bold};
  for (int i = 0; i < 4; ++i) {
    std::cout << "    " << std::left << std::setw(16) << names[i] << std::right
              << std::setw(10) << (plans[i]->found ? "yes" : "no") << std::fixed
              << std::setprecision(2) << std::setw(12);
    if (plans[i]->found) std::cout << path_length(plans[i]->cells);
    else std::cout << "-";
    std::cout << std::setw(18) << unseen_cells(*plans[i]) << "\n";
  }

  std::cout << "\n    where the dial turns over\n\n";
  std::cout << "    " << std::right << std::setw(16) << "multiplier"
            << std::setw(12) << "length" << std::setw(18) << "unseen cells" << "\n";
  for (const double multiplier : {1.00, 1.05, 1.10, 1.15, 1.20, 2.00}) {
    PlanOptions dial;
    dial.unknown = UnknownIs::expensive;
    dial.unknown_multiplier = multiplier;
    const Plan plan = plan_path(grid, start, goal, dial);
    std::cout << "    " << std::right << std::fixed << std::setprecision(2)
              << std::setw(16) << multiplier << std::setw(12)
              << path_length(plan.cells) << std::setw(18) << unseen_cells(plan) << "\n";
  }

  std::cout << "\n    the bold plan is shorter because it drives through ground\n";
  std::cout << "    no sensor has ever seen, and it is shorter for that reason\n";
  std::cout << "    alone\n";

  // Treating unknown as free routes straight through the unseen half.
  RC_REQUIRE(bold.found);
  RC_CHECK(unseen_cells(bold) > 0);

  // Treating it as blocked keeps every cell of the route on surveyed ground.
  RC_REQUIRE(careful.found);
  RC_CHECK_EQ(unseen_cells(careful), 0);
  RC_CHECK(path_length(careful.cells) > path_length(bold.cells));

  // And expensive is a dial rather than a third opinion. At four times the
  // cost the shortcut is not worth taking and the route is the careful one; at
  // 1.2 it is worth taking and the route is the bold one. The multiplier is
  // where the actual decision lives, and it deserves a number somebody argued
  // about rather than a default.
  RC_REQUIRE(cautious.found);
  RC_REQUIRE(tempted.found);
  RC_CHECK_EQ(unseen_cells(cautious), 0);
  RC_CHECK(unseen_cells(tempted) > 0);
}

RC_TEST("a guess that overestimates is faster and no longer optimal") {
  const OccupancyGrid room = surveyed_room();
  const GridPoint start{2, 20}, goal{57, 20};

  std::cout << "\n    the same room, searched three ways\n\n";
  std::cout << "    " << std::left << std::setw(14) << "guess" << std::right
            << std::setw(12) << "cost" << std::setw(16) << "over optimal"
            << std::setw(12) << "expanded" << "\n";

  PlanOptions options;
  options.guess = Guess::none;
  const Plan optimal = plan_path(room, start, goal, options);
  RC_REQUIRE(optimal.found);

  const char* names[] = {"none", "octile", "manhattan"};
  const Guess guesses[] = {Guess::none, Guess::octile, Guess::manhattan};
  for (int i = 0; i < 3; ++i) {
    options.guess = guesses[i];
    const Plan plan = plan_path(room, start, goal, options);
    RC_REQUIRE(plan.found);
    std::cout << "    " << std::left << std::setw(14) << names[i] << std::right
              << std::fixed << std::setprecision(4) << std::setw(12) << plan.cost
              << std::setprecision(2) << std::setw(15)
              << (plan.cost / optimal.cost - 1.0) * 100.0 << "%" << std::setw(12)
              << plan.expanded << "\n";
  }

  // Octile is admissible, so it finds the same path as an exhaustive search
  // while looking at half as many cells.
  options.guess = Guess::octile;
  const Plan honest = plan_path(room, start, goal, options);
  RC_CHECK_NEAR(honest.cost, optimal.cost, 1e-9);
  RC_CHECK(honest.expanded < optimal.expanded);

  // On this particular map Manhattan happens to find the optimal path too, and
  // faster. That is luck rather than a property, so the next block asks the
  // question properly.
  std::cout << "\n    on this map the overestimate got away with it, so ask\n";
  std::cout << "    two hundred maps instead\n\n";

  int tried = 0, worse = 0;
  double worst_overshoot = 0.0;
  long long octile_expanded = 0, manhattan_expanded = 0;
  for (unsigned seed = 1; seed <= 200; ++seed) {
    const OccupancyGrid clutter = cluttered_room(seed, 0.28);
    if (clutter.classify(start.x, start.y) != rc::nav::Cell::free) continue;
    if (clutter.classify(goal.x, goal.y) != rc::nav::Cell::free) continue;

    options.guess = Guess::none;
    const Plan best = plan_path(clutter, start, goal, options);
    if (!best.found) continue;

    options.guess = Guess::octile;
    const Plan admissible = plan_path(clutter, start, goal, options);
    options.guess = Guess::manhattan;
    const Plan greedy = plan_path(clutter, start, goal, options);
    if (!greedy.found || !admissible.found) continue;

    ++tried;
    octile_expanded += admissible.expanded;
    manhattan_expanded += greedy.expanded;

    // The admissible guess is optimal on every one of them.
    RC_CHECK_NEAR(admissible.cost, best.cost, 1e-9);

    const double overshoot = greedy.cost / best.cost - 1.0;
    if (overshoot > 1e-9) {
      ++worse;
      if (overshoot > worst_overshoot) worst_overshoot = overshoot;
    }
  }

  std::cout << "    " << std::left << std::setw(34) << "maps searched" << std::right
            << tried << "\n";
  std::cout << "    " << std::left << std::setw(34) << "where manhattan was suboptimal"
            << std::right << worse << "\n";
  std::cout << "    " << std::left << std::setw(34) << "worst it was out by"
            << std::right << std::fixed << std::setprecision(2)
            << worst_overshoot * 100.0 << "%\n";
  std::cout << "    " << std::left << std::setw(34) << "cells expanded, octile"
            << std::right << octile_expanded << "\n";
  std::cout << "    " << std::left << std::setw(34) << "cells expanded, manhattan"
            << std::right << manhattan_expanded << "\n";

  std::cout << "\n    a few percent longer on three maps in ten, for a fifth\n";
  std::cout << "    fewer cells looked at. That is a trade, and it has to be\n";
  std::cout << "    made on purpose rather than by picking the obvious formula\n";

  RC_CHECK(tried > 100);
  RC_CHECK(worse > tried / 10);        // it is not rare
  RC_CHECK(worst_overshoot < 0.10);    // and it is not catastrophic either
  RC_CHECK(manhattan_expanded < octile_expanded);
}

RC_TEST("no route at all is an answer, not a crash") {
  OccupancyGrid grid(20, 20, 0.10, 0.0, 0.0);
  for (int y = 0; y < 20; ++y)
    for (int x = 0; x < 20; ++x) make_free(grid, x, y);
  for (int y = 0; y < 20; ++y) make_occupied(grid, 10, y);

  const Plan plan = plan_path(grid, GridPoint{2, 10}, GridPoint{18, 10}, PlanOptions{});
  RC_CHECK(!plan.found);
  RC_CHECK(plan.cells.empty());
  RC_CHECK(plan.expanded > 0);   // it did look

  // A goal outside the map, and a start outside it, are both refused rather
  // than indexed.
  RC_CHECK(!plan_path(grid, GridPoint{2, 10}, GridPoint{99, 10}, PlanOptions{}).found);
  RC_CHECK(!plan_path(grid, GridPoint{-1, 0}, GridPoint{5, 5}, PlanOptions{}).found);

  // A goal on top of the start is a path of one cell and no distance.
  const Plan nowhere = plan_path(grid, GridPoint{5, 5}, GridPoint{5, 5}, PlanOptions{});
  RC_CHECK(nowhere.found);
  RC_CHECK_EQ(static_cast<int>(nowhere.cells.size()), 1);
  RC_CHECK_NEAR(path_length(nowhere.cells), 0.0, 1e-12);
}
