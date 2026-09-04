#include <rc/test/rc_test.hpp>

#include <cmath>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

#include "solution.hpp"

namespace {

// The map as it would be kept if cells held probabilities and observations
// multiplied, which is the arrangement this lesson exists to argue against.
double multiply_in(double probability, bool occupied) {
  const double likelihood = occupied ? 0.7 : 0.3;
  const double numerator = probability * likelihood;
  const double denominator = numerator + (1.0 - probability) * (1.0 - likelihood);
  return denominator > 0.0 ? numerator / denominator : probability;
}

// How many contrary observations bring a cell back past the halfway mark.
int misses_to_recover_multiplied(int hits, int give_up = 5000) {
  double p = 0.5;
  for (int i = 0; i < hits; ++i) p = multiply_in(p, true);
  int misses = 0;
  while (p >= 0.5 && misses < give_up) {
    p = multiply_in(p, false);
    ++misses;
  }
  return misses >= give_up ? -1 : misses;
}

int misses_to_recover_log_odds(int hits, int give_up = 5000) {
  OccupancyGrid grid(4, 4, 0.1, 0.0, 0.0);
  for (int i = 0; i < hits; ++i) grid.observe(1, 1, true);
  int misses = 0;
  while (grid.log_odds(1, 1) >= 0.0 && misses < give_up) {
    grid.observe(1, 1, false);
    ++misses;
  }
  return misses >= give_up ? -1 : misses;
}

std::string or_never(int value) { return value < 0 ? "never" : std::to_string(value); }

}  // namespace

RC_TEST("a point in the world lands in one cell, on both sides of the origin") {
  // Ten by ten cells of 10 cm, with the origin in the middle of the world.
  OccupancyGrid grid(10, 10, 0.10, -0.5, -0.5);

  std::cout << "\n    a world point to a cell index, 10 cm cells, origin at -0.5\n\n";
  std::cout << "    " << std::right << std::setw(10) << "x" << std::setw(12) << "floored"
            << std::setw(14) << "truncated" << "\n";

  bool differed = false;
  for (const double x : {-0.45, -0.25, -0.05, 0.05, 0.25, 0.45}) {
    const CellIndex index = grid.cell_for(x, 0.0);
    const int truncated = static_cast<int>((x - (-0.5)) / 0.10);
    if (index.x != truncated) differed = true;
    std::cout << "    " << std::right << std::fixed << std::setprecision(2)
              << std::setw(10) << x << std::setw(12) << index.x << std::setw(14)
              << truncated << "\n";
  }

  // Every 10 cm of world is one cell wide, on both sides of the origin.
  RC_CHECK_EQ(grid.cell_for(-0.45, 0.0).x, 0);
  RC_CHECK_EQ(grid.cell_for(-0.35, 0.0).x, 1);
  RC_CHECK_EQ(grid.cell_for(0.45, 0.0).x, 9);

  // A point outside the grid is reported rather than silently clamped into it.
  RC_CHECK(!grid.cell_for(2.0, 0.0).inside);
  RC_CHECK(!grid.cell_for(0.0, -3.0).inside);
  RC_CHECK(grid.cell_for(0.0, 0.0).inside);

  // With this origin nothing above is negative, so truncation happens to agree.
  // Move the origin to the centre of the world, which is where it usually is,
  // and it stops agreeing.
  OccupancyGrid centred(10, 10, 0.10, 0.0, 0.0);
  std::cout << "\n    the same, with the origin at the centre of the world\n\n";
  std::cout << "    " << std::right << std::setw(10) << "x" << std::setw(12) << "floored"
            << std::setw(14) << "truncated" << "\n";
  for (const double x : {-0.25, -0.15, -0.05, 0.05, 0.15, 0.25}) {
    const int floored = centred.cell_for(x, 0.0).x;
    const int truncated = static_cast<int>(x / 0.10);
    if (floored != truncated) differed = true;
    std::cout << "    " << std::right << std::fixed << std::setprecision(2)
              << std::setw(10) << x << std::setw(12) << floored << std::setw(14)
              << truncated << "\n";
  }

  std::cout << "\n    truncated, -0.05 and +0.05 are the same cell, so the cell\n";
  std::cout << "    at the origin is 0.20 m wide and every other one is 0.10\n";

  RC_CHECK(differed);
  RC_CHECK_EQ(centred.cell_for(-0.05, 0.0).x, -1);
  RC_CHECK_EQ(centred.cell_for(0.05, 0.0).x, 0);
}

RC_TEST("a cell nobody has looked at is not free") {
  OccupancyGrid grid(10, 10, 0.10, 0.0, 0.0);

  RC_CHECK(grid.classify(5, 5) == Cell::unknown);
  RC_CHECK_NEAR(grid.probability(5, 5), 0.5, 1e-12);

  // One observation is not enough to decide, which is deliberate: a single
  // reading from a sensor right seven times in ten is not an answer.
  grid.observe(5, 5, true);
  RC_CHECK(grid.classify(5, 5) == Cell::unknown);

  grid.observe(5, 5, true);
  RC_CHECK(grid.classify(5, 5) == Cell::occupied);

  // And the same in the other direction.
  OccupancyGrid other(10, 10, 0.10, 0.0, 0.0);
  other.observe(2, 2, false);
  other.observe(2, 2, false);
  RC_CHECK(other.classify(2, 2) == Cell::free);

  // Evidence that cancels returns to not knowing, rather than to whichever
  // answer arrived last.
  other.observe(2, 2, true);
  other.observe(2, 2, true);
  RC_CHECK(other.classify(2, 2) == Cell::unknown);
  RC_CHECK_NEAR(other.log_odds(2, 2), 0.0, 1e-12);

  // Outside the grid is unknown, and asking does not corrupt anything.
  RC_CHECK(grid.classify(-1, 0) == Cell::unknown);
  grid.observe(-1, 0, true);
  RC_CHECK(grid.classify(-1, 0) == Cell::unknown);
}

RC_TEST("a map that can be told it was wrong") {
  std::cout << "\n    a cell observed occupied N times, then contradicted\n\n";
  std::cout << "    " << std::right << std::setw(10) << "hits" << std::setw(20)
            << "1 - p, multiplied" << std::setw(16) << "misses back" << std::setw(18)
            << "log odds back" << "\n";

  for (const int hits : {1, 10, 40, 43, 44, 100, 2000}) {
    double p = 0.5;
    for (int i = 0; i < hits; ++i) p = multiply_in(p, true);
    std::cout << "    " << std::right << std::setw(10) << hits << std::setw(20)
              << std::scientific << std::setprecision(3) << 1.0 - p << std::setw(16)
              << or_never(misses_to_recover_multiplied(hits)) << std::setw(18)
              << or_never(misses_to_recover_log_odds(hits)) << "\n";
  }

  std::cout << "\n    at 44 observations one minus p is smaller than a double\n";
  std::cout << "    can hold beside one, so p is exactly 1 and nothing will\n";
  std::cout << "    ever move it again\n";

  // Forty three still recovers. Forty four never does.
  RC_CHECK(misses_to_recover_multiplied(43) > 0);
  RC_CHECK_EQ(misses_to_recover_multiplied(44), -1);
  RC_CHECK_EQ(misses_to_recover_multiplied(2000), -1);

  // Clamped log odds recover in the same five observations however sure the
  // cell had become, which is what the clamp is for.
  RC_CHECK_EQ(misses_to_recover_log_odds(10), misses_to_recover_log_odds(2000));
  RC_CHECK_EQ(misses_to_recover_log_odds(2000), 5);

  // And the clamp really does bound it.
  OccupancyGrid grid(4, 4, 0.1, 0.0, 0.0);
  for (int i = 0; i < 1000; ++i) grid.observe(1, 1, true);
  RC_CHECK_NEAR(grid.log_odds(1, 1), OccupancyGrid::kClamp, 1e-12);
  RC_CHECK(grid.probability(1, 1) < 1.0);
}

RC_TEST("a beam marks what it passed through, and only where it stopped") {
  OccupancyGrid grid(40, 40, 0.10, 0.0, 0.0);

  // A sensor at (0.05, 0.05), which is cell (0, 0), seeing a wall 2 m away.
  for (int scan = 0; scan < 4; ++scan) grid.integrate_ray(0.05, 0.05, 2.05, 0.05, true);

  RC_CHECK(grid.classify(20, 0) == Cell::occupied);
  RC_CHECK(grid.classify(10, 0) == Cell::free);
  RC_CHECK(grid.classify(1, 0) == Cell::free);

  // Nothing beyond the wall was touched. The beam did not go there.
  RC_CHECK(grid.classify(25, 0) == Cell::unknown);

  // Nor was anything off the line.
  RC_CHECK(grid.classify(10, 5) == Cell::unknown);

  // A reading that ran out to the sensor's maximum without returning says the
  // space was empty and says nothing at all about its far end. Marking that end
  // occupied would paint a wall at exactly max range around every place the
  // robot has ever stood.
  OccupancyGrid open(40, 40, 0.10, 0.0, 0.0);
  for (int scan = 0; scan < 4; ++scan) open.integrate_ray(0.05, 0.05, 2.05, 0.05, false);
  RC_CHECK(open.classify(10, 0) == Cell::free);
  RC_CHECK(open.classify(20, 0) != Cell::occupied);
}

RC_TEST("the wall comes down when it is taken away") {
  OccupancyGrid grid(40, 40, 0.10, 0.0, 0.0);

  // A van parks 2 m in front of the robot and stays for two hundred scans.
  for (int scan = 0; scan < 200; ++scan) grid.integrate_ray(0.05, 0.05, 2.05, 0.05, true);
  RC_CHECK(grid.classify(20, 0) == Cell::occupied);

  std::cout << "\n    after 200 scans of a parked van, log odds "
            << std::fixed << std::setprecision(4) << grid.log_odds(20, 0) << "\n";

  // It drives away. The beam now reaches a wall at 3 m.
  int scans_to_clear = 0;
  for (int scan = 0; scan < 200; ++scan) {
    grid.integrate_ray(0.05, 0.05, 3.05, 0.05, true);
    ++scans_to_clear;
    if (grid.classify(20, 0) == Cell::free) break;
  }

  std::cout << "    scans after it left before the map agreed: " << scans_to_clear << "\n";
  std::cout << "\n    two hundred scans of evidence, undone by six. Without the\n";
  std::cout << "    clamp it would have taken two hundred, and with probabilities\n";
  std::cout << "    multiplied it would never have happened at all\n";

  RC_CHECK(grid.classify(20, 0) == Cell::free);
  RC_CHECK(scans_to_clear <= 8);

  // And the new wall is where the new wall is.
  RC_CHECK(grid.classify(30, 0) == Cell::occupied);
}
