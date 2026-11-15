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
    {"alpha", 0.1},
    {"eps", 0.1},
    {"cells", 17.0},
    {"agree_100", 12.0},
    {"agree_500", 15.0},
    {"agree_2000", 17.0},
    {"goal_100", 48.35},
    {"goal_500", 49.95},
    {"goal_2000", 54.25},
    {"goal_opt", 54.25},
    {"pits", 759.0},
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

RC_TEST("one update moves a move's value a share alpha of the way to its target, and a run that ends adds no future") {
  const GridTask g = pit_grid();
  std::vector<double> Q = make_q(g, 0.0);
  const int s = start_of(g), next = s + 1;
  Q[static_cast<std::size_t>(next) * 4 + 2] = 4.0;   // the best value in next
  q_update(Q, s, 1, -1.0, next, false, 0.5, 0.9);
  RC_CHECK(std::fabs(Q[static_cast<std::size_t>(s) * 4 + 1] - 0.5 * (-1.0 + 0.9 * 4.0)) < 1e-15);
  q_update(Q, s, 0, 10.0, next, true, 0.5, 0.9);
  RC_CHECK(std::fabs(Q[static_cast<std::size_t>(s) * 4 + 0] - 5.0) < 1e-15);
  // the greedy choice follows the table; epsilon 1 is uniformly random
  RC_CHECK(best_move(Q, s) == 0);
  Rng r = make_rng(1);
  int counts[4] = {0, 0, 0, 0};
  for (int i = 0; i < 4000; ++i) ++counts[choose_move(Q, s, 1.0, r)];
  for (int m = 0; m < 4; ++m) RC_CHECK(counts[m] > 850 && counts[m] < 1150);
}

RC_TEST("learning only from trying, the policy comes to agree with the optimum of value iteration as the episodes add up") {
  const GridTask g = pit_grid();
  const double gamma = 0.95, alpha = 0.1, eps = 0.1;
  const std::vector<int> opt = greedy_policy(g, value_iteration(g, gamma, 1e-10, nullptr), gamma);
  int cells = 0;
  for (int s = 0; s < cell_count(g); ++s)
    if (!is_wall(g, s) && !is_terminal(g, s)) ++cells;
  const int counts[3] = {100, 500, 2000};
  double agree[3], goal[3];
  for (int k = 0; k < 3; ++k) {
    const std::vector<int> p = q_policy(g, q_learning(g, counts[k], alpha, gamma, eps, 0.0, 3, nullptr));
    int same = 0;
    for (int s = 0; s < cell_count(g); ++s)
      if (!is_wall(g, s) && !is_terminal(g, s) && p[static_cast<std::size_t>(s)] == opt[static_cast<std::size_t>(s)]) ++same;
    agree[k] = same;
    goal[k] = outcome_stats(g, p, 4000, 7).goal_share * 100.0;
  }
  rec("gamma", gamma, 2);
  rec("alpha", alpha, 1);
  rec("eps", eps, 1);
  rec("cells", cells, 0);
  rec("agree_100", agree[0], 0);
  rec("agree_500", agree[1], 0);
  rec("agree_2000", agree[2], 0);
  rec("goal_100", goal[0], 0);
  rec("goal_500", goal[1], 0);
  rec("goal_2000", goal[2], 0);
  rec("goal_opt", outcome_stats(g, opt, 4000, 7).goal_share * 100.0, 0);
  RC_CHECK(agree[2] >= agree[0]);
  RC_CHECK(agree[2] >= cells - 3);
}

RC_TEST("the trials are real: while it learned, the learner fell into a pit many times") {
  const GridTask g = pit_grid();
  QStats st;
  q_learning(g, 2000, 0.1, 0.95, 0.1, 0.0, 3, &st);
  rec("pits", st.pits, 0);
  RC_CHECK(st.goals + st.pits + st.timeouts == 2000);
  RC_CHECK(st.pits > 50);
}
