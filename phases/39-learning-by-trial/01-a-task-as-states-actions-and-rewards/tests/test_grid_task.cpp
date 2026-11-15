#include <rc/test/rc_test.hpp>
#include <sstream>
#include <cstring>

#include <cmath>

#include "solution.hpp"
#include <iomanip>
#include <iostream>

// The numbers this lesson publishes: its page, its header and its quiz quote them, so each is held here as it was
// measured. A change that moves one by more than half a unit of its last printed digit fails the suite.
struct Pin {
  const char* key;
  double value;
};
static const Pin kPins[] = {
    {"slip", 0.1},
    {"step", -1.0},
    {"gamma", 0.95},
    {"v_random", -13.275664670504755},
    {"sweeps_random", 221.0},
    {"v_east", -5.852030338142722},
    {"v_round", -4.0415273311203395},
    {"runs", 20000.0},
    {"mc_east", -5.835937079785723},
    {"pit_share", 47.73},
};

static void rec(const char* key, double value, int digits = 3) {
  std::cout << "@" << key << " " << std::fixed << std::setprecision(digits) << value << std::endl;
  std::cout.unsetf(std::ios::floatfield);
  for (const Pin& pin : kPins) {
    if (std::strcmp(pin.key, key) != 0) continue;
    double allowed = 0.5;
    for (int i = 0; i < digits; ++i) allowed /= 10.0;
    if (!(std::fabs(value - pin.value) <= allowed)) {
      std::ostringstream message;
      message << "the published number " << key << " was " << pin.value << " and is now " << value;
      rc::test::record(__FILE__, __LINE__, message.str());
    }
  }
}
[[maybe_unused]] static void recs(const char* key, const std::string& value) { std::cout << "@" << key << " " << value << std::endl; }


// A corridor along a row of pits, and a longer way round above it.
static GridTask pit_grid() {
  return make_grid({"#########",
                    "#.......#",
                    "#.#####.#",
                    "#S.....G#",
                    "#PPPPPPP#",
                    "#########"}, -1.0, 0.1);
}

// The same move in every cell.
static std::vector<int> always(const GridTask& g, int m) { return std::vector<int>(g.cells.size(), m); }

// North from the start, east along the top and south down to the goal (a cell of the corridor, reached only by a slip, goes on east).
static std::vector<int> round_above(const GridTask& g) {
  std::vector<int> p(g.cells.size(), kEast);
  for (int y = 0; y < g.height; ++y) {
    for (int x = 0; x < g.width; ++x) {
      const int s = y * g.width + x;
      if (x == 1) p[static_cast<std::size_t>(s)] = kNorth;
      if (x == 7) p[static_cast<std::size_t>(s)] = kSouth;
      if (y == 1 && x >= 1 && x < 7) p[static_cast<std::size_t>(s)] = kEast;
    }
  }
  return p;
}

RC_TEST("the grid is read from text: its size, its start, which cells end a run, and a move into a wall stays put") {
  const GridTask g = pit_grid();
  RC_CHECK(g.width == 9 && g.height == 6);
  RC_CHECK(start_of(g) == 3 * 9 + 1);
  RC_CHECK(is_terminal(g, 3 * 9 + 7) && is_terminal(g, 4 * 9 + 3) && !is_terminal(g, start_of(g)));
  RC_CHECK(moved(g, start_of(g), kWest) == start_of(g));
  RC_CHECK(moved(g, start_of(g), kEast) == start_of(g) + 1);
  RC_CHECK(reward_of(g, 3 * 9 + 7) == 10.0 && reward_of(g, 4 * 9 + 2) == -10.0);
  double c[3];
  int n[3];
  outcomes(g, start_of(g), kEast, c, n);
  RC_CHECK(std::fabs(c[0] + c[1] + c[2] - 1.0) < 1e-15);
  RC_CHECK(n[1] == 4 * 9 + 1);   // a slip to the south from the start is a pit
  rec("slip", g.slip, 1);
  rec("step", g.step, 0);
}

RC_TEST("the value of a policy is found by sweeping until it settles, and the random policy is worth little") {
  const GridTask g = pit_grid();
  const double gamma = 0.95;
  int sweeps = 0;
  const std::vector<double> vr = evaluate_random(g, gamma, 1e-9, &sweeps);
  rec("gamma", gamma, 2);
  rec("v_random", vr[static_cast<std::size_t>(start_of(g))], 3);
  rec("sweeps_random", sweeps, 0);
  int se = 0, so = 0;
  const std::vector<double> ve = evaluate_policy(g, always(g, kEast), gamma, 1e-9, &se);
  const std::vector<double> vo = evaluate_policy(g, round_above(g), gamma, 1e-9, &so);
  rec("v_east", ve[static_cast<std::size_t>(start_of(g))], 3);
  rec("v_round", vo[static_cast<std::size_t>(start_of(g))], 3);
  RC_CHECK(ve[static_cast<std::size_t>(start_of(g))] > vr[static_cast<std::size_t>(start_of(g))]);
  RC_CHECK(se > 1 && se < 100000);
  // a settled value satisfies the equation it was swept with
  const std::size_t s = static_cast<std::size_t>(start_of(g) + 2);
  RC_CHECK(std::fabs(ve[s] - move_value(g, ve, static_cast<int>(s), kEast, gamma)) < 1e-8);
}

RC_TEST("the value is what running the policy gets on average: many runs agree with it, and some of them end in a pit") {
  const GridTask g = pit_grid();
  const double gamma = 0.95;
  const std::vector<int> p = always(g, kEast);
  const std::vector<double> v = evaluate_policy(g, p, gamma, 1e-9, nullptr);
  Rng r = make_rng(7);
  const int runs = 20000;
  double sum = 0.0;
  int pits = 0;
  for (int i = 0; i < runs; ++i) {
    char ended = '-';
    sum += run_policy(g, p, gamma, 200, r, &ended);
    if (ended == 'P') ++pits;
  }
  rec("runs", runs, 0);
  rec("mc_east", sum / runs, 3);
  rec("pit_share", 100.0 * pits / runs, 1);
  RC_CHECK(std::fabs(sum / runs - v[static_cast<std::size_t>(start_of(g))]) < 0.1);
  RC_CHECK(pits > 0);
}
