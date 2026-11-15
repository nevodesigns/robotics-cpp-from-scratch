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
    {"gamma", 0.95},
    {"sweeps", 36.0},
    {"v_opt", -5.24498113108685},
    {"v_random", -13.356982345786221},
    {"v_round", -5.745},
    {"g_low", 0.8},
    {"g_high", 0.95},
    {"goal_low", 95.25},
    {"ret_low", -5.129},
    {"goal_high", 54.25},
    {"ret_high", -5.1645},
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


static GridTask pit_grid() {
  return make_grid({"#########",
                    "#.......#",
                    "#.#####.#",
                    "#.#####.#",
                    "#S.....G#",
                    "#PPPPPPP#",
                    "#########"}, -1.0, 0.1);
}

RC_TEST("value iteration settles on a value no policy beats, and the greedy policy it gives is worth exactly that") {
  const GridTask g = pit_grid();
  const double gamma = 0.95;
  int sweeps = 0;
  const std::vector<double> V = value_iteration(g, gamma, 1e-10, &sweeps);
  const std::size_t s0 = static_cast<std::size_t>(start_of(g));
  rec("gamma", gamma, 2);
  rec("sweeps", sweeps, 0);
  rec("v_opt", V[s0], 3);
  const std::vector<int> p = greedy_policy(g, V, gamma);
  const std::vector<double> vp = evaluate_policy(g, p, gamma, 1e-10, nullptr);
  RC_CHECK(std::fabs(vp[s0] - V[s0]) < 1e-6);
  // better than the random policy and than always east, everywhere
  const std::vector<double> vr = evaluate_random(g, gamma, 1e-10, nullptr);
  const std::vector<double> ve = evaluate_policy(g, std::vector<int>(g.cells.size(), kEast), gamma, 1e-10, nullptr);
  for (int s = 0; s < cell_count(g); ++s) {
    if (is_wall(g, s) || is_terminal(g, s)) continue;
    RC_CHECK(V[static_cast<std::size_t>(s)] >= vr[static_cast<std::size_t>(s)] - 1e-9);
    RC_CHECK(V[static_cast<std::size_t>(s)] >= ve[static_cast<std::size_t>(s)] - 1e-9);
  }
  rec("v_random", vr[s0], 3);
  rec("v_round", -5.745, 3);   // the value of going round, from 39-01
  RC_CHECK(V[s0] >= -5.745);
}

RC_TEST("the discount chooses the route: a low one goes the long way round, a high one takes the corridor beside the pits") {
  const GridTask g = pit_grid();
  const std::size_t s0 = static_cast<std::size_t>(start_of(g));
  const double low = 0.8, high = 0.95;
  const std::vector<int> pl = greedy_policy(g, value_iteration(g, low, 1e-10, nullptr), low);
  const std::vector<int> ph = greedy_policy(g, value_iteration(g, high, 1e-10, nullptr), high);
  RC_CHECK(pl[s0] == kNorth);
  RC_CHECK(ph[s0] == kEast);
  const OutcomeStats ol = outcome_stats(g, pl, 4000, 7), oh = outcome_stats(g, ph, 4000, 7);
  rec("g_low", low, 2);
  rec("g_high", high, 2);
  rec("goal_low", ol.goal_share * 100.0, 0);
  rec("ret_low", ol.mean_return, 2);
  rec("goal_high", oh.goal_share * 100.0, 0);
  rec("ret_high", oh.mean_return, 2);
  RC_CHECK(ol.goal_share > oh.goal_share + 0.2);
}
