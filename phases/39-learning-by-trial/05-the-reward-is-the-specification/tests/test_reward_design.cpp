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
    {"best", 4.0},
    {"few", 20.0},
    {"many", 200.0},
    {"limit", 200.0},
    {"plain_few", 200.0},
    {"pot_few", 4.0},
    {"plain_many", 8.0},
    {"pot_many", 4.0},
    {"bonus_few", 200.0},
    {"bonus_many", 200.0},
    {"bonus_paid", 101.0},
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


// No step cost, no slip, +10 at the goal.
static GridTask open_grid() {
  return make_grid({"#######",
                    "#.....#",
                    "#.....#",
                    "#S...G#",
                    "#######"}, 0.0, 0.0);
}

RC_TEST("each reward pays what it says: the plain one only at the goal, the potential a hint whose discounted sum depends only on where a run starts and ends, the bonus for every step closer") {
  const GridTask g = open_grid();
  const int s = start_of(g), right = s + 1;
  RC_CHECK(shaped_reward(g, s, right, kPlain, 0.99) == 0.0);
  RC_CHECK(shaped_reward(g, s, right, kCloser, 0.99) == 1.0);
  RC_CHECK(shaped_reward(g, right, s, kCloser, 0.99) == 0.0);
  // the potential's hint on a step closer is positive, and along any path the discounted hints add up to
  // gamma^T times the potential at the end less the potential at the start, whatever the route
  const double gamma = 0.99;
  RC_CHECK(shaped_reward(g, s, right, kPotential, gamma) > 0.0);
  const int path[6] = {s, right, right - g.width, right - g.width + 1, right + 1, right};
  double sum = 0.0, disc = 1.0;
  for (int k = 0; k + 1 < 6; ++k) {
    sum += disc * (shaped_reward(g, path[k], path[k + 1], kPotential, gamma) - shaped_reward(g, path[k], path[k + 1], kPlain, gamma));
    disc *= gamma;
  }
  const double expect = disc * -static_cast<double>(goal_distance(g, path[5])) + static_cast<double>(goal_distance(g, path[0]));
  RC_CHECK(std::fabs(sum - expect) < 1e-12);
  // arriving at the goal is paid its +10, and the potential there is zero
  const int goal = static_cast<int>(g.cells.find('G'));
  RC_CHECK(std::fabs(shaped_reward(g, goal - 1, goal, kPotential, 0.99) - (10.0 + 1.0)) < 1e-12);
}

RC_TEST("the potential's hint finds the shortest way after a few episodes, where the plain reward has not yet found the goal") {
  const GridTask g = open_grid();
  const int few = 20, many = 200, limit = 200;
  const double gamma = 0.99;
  rec("best", goal_distance(g, start_of(g)), 0);
  rec("few", few, 0);
  rec("many", many, 0);
  rec("limit", limit, 0);
  const int pf = steps_to_goal(g, q_learning_shaped(g, few, 0.2, gamma, 0.1, kPlain, 3), limit);
  const int tf = steps_to_goal(g, q_learning_shaped(g, few, 0.2, gamma, 0.1, kPotential, 3), limit);
  const int pm = steps_to_goal(g, q_learning_shaped(g, many, 0.2, gamma, 0.1, kPlain, 3), limit);
  const int tm = steps_to_goal(g, q_learning_shaped(g, many, 0.2, gamma, 0.1, kPotential, 3), limit);
  rec("plain_few", pf, 0);
  rec("pot_few", tf, 0);
  rec("plain_many", pm, 0);
  rec("pot_many", tm, 0);
  RC_CHECK(tf == goal_distance(g, start_of(g)));
  RC_CHECK(tm == goal_distance(g, start_of(g)));
  RC_CHECK(pf > tf);
  RC_CHECK(pm < limit);
}

RC_TEST("the bonus for getting closer is gamed: the learner steps away and back for ever and never arrives") {
  const GridTask g = open_grid();
  const int limit = 200;
  const std::vector<double> qf = q_learning_shaped(g, 20, 0.2, 0.99, 0.1, kCloser, 3);
  const std::vector<double> qm = q_learning_shaped(g, 200, 0.2, 0.99, 0.1, kCloser, 3);
  rec("bonus_few", steps_to_goal(g, qf, limit), 0);
  rec("bonus_many", steps_to_goal(g, qm, limit), 0);
  RC_CHECK(steps_to_goal(g, qm, limit) == limit);
  // what it collects: follow its policy and add up the bonus
  int s = start_of(g);
  double paid = 0.0;
  for (int t = 0; t < limit; ++t) {
    const int next = moved(g, s, best_move(qm, s));
    paid += shaped_reward(g, s, next, kCloser, 0.99) - shaped_reward(g, s, next, kPlain, 0.99);
    s = next;
  }
  rec("bonus_paid", paid, 0);
  RC_CHECK(paid > 50.0);
}
