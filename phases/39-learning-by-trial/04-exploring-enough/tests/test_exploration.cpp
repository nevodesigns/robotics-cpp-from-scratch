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
    {"episodes", 2000.0},
    {"big_eps0", 1.0},
    {"ret_eps0", 1.0},
    {"far_eps0", 2.0},
    {"big_eps1", 2.0},
    {"ret_eps1", 1.0},
    {"far_eps1", 3.0},
    {"big_eps5", 27.0},
    {"ret_eps5", 1.0},
    {"far_eps5", 44.0},
    {"q0", 20.0},
    {"big_opt", 1970.0},
    {"ret_opt", 4.0},
    {"far_opt", 2155.0},
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


// A goal worth 3 two steps up from the start, one worth 10 at the end of the corridor; no slip.
static GridTask lure_grid() {
  return make_grid({"#########",
                    "#g......#",
                    "#.#####.#",
                    "#S.....G#",
                    "#########"}, -1.0, 0.0);
}

RC_TEST("with no random moves, or a tenth of them, the learner settles for the small goal next to the start") {
  const GridTask g = lure_grid();
  const int episodes = 2000;
  std::vector<int> v0, v1;
  int b0 = 0, b1 = 0;
  const std::vector<double> q0 = q_learning_visits(g, episodes, 0.1, 0.95, 0.0, 0.0, 3, v0, &b0);
  const std::vector<double> q1 = q_learning_visits(g, episodes, 0.1, 0.95, 0.1, 0.0, 3, v1, &b1);
  char e0 = '-', e1 = '-';
  rec("episodes", episodes, 0);
  rec("big_eps0", b0, 0);
  rec("ret_eps0", greedy_return(g, q0, &e0), 0);
  rec("far_eps0", v0[static_cast<std::size_t>(3 * 9 + 6)], 0);
  rec("big_eps1", b1, 0);
  rec("ret_eps1", greedy_return(g, q1, &e1), 0);
  rec("far_eps1", v1[static_cast<std::size_t>(3 * 9 + 6)], 0);
  RC_CHECK(e0 == 'g' && e1 == 'g');
  RC_CHECK(b0 + b1 < 20);
  int total = 0;
  for (int x : v1) total += x;
  RC_CHECK(total > episodes);
}

RC_TEST("half the moves random gets down the corridor more often, and still not often enough to learn that the large goal is better") {
  const GridTask g = lure_grid();
  std::vector<int> v;
  int b = 0;
  const std::vector<double> q = q_learning_visits(g, 2000, 0.1, 0.95, 0.5, 0.0, 3, v, &b);
  char e = '-';
  rec("big_eps5", b, 0);
  rec("ret_eps5", greedy_return(g, q, &e), 0);
  rec("far_eps5", v[static_cast<std::size_t>(3 * 9 + 6)], 0);
  std::vector<int> v1;
  q_learning_visits(g, 2000, 0.1, 0.95, 0.1, 0.0, 3, v1, nullptr);
  RC_CHECK(v[static_cast<std::size_t>(3 * 9 + 6)] > v1[static_cast<std::size_t>(3 * 9 + 6)]);
}

RC_TEST("values started higher than anything the task can give send the learner to every untried move, and it finds the large goal with no random moves") {
  const GridTask g = lure_grid();
  const double q0 = 20.0;
  std::vector<int> v;
  int b = 0;
  const std::vector<double> q = q_learning_visits(g, 2000, 0.1, 0.95, 0.0, q0, 3, v, &b);
  char e = '-';
  const double ret = greedy_return(g, q, &e);
  rec("q0", q0, 0);
  rec("big_opt", b, 0);
  rec("ret_opt", ret, 0);
  RC_CHECK(e == 'G');
  RC_CHECK(ret == 4.0);   // six steps east at -1 each, then +10
  rec("far_opt", v[static_cast<std::size_t>(3 * 9 + 6)], 0);
  RC_CHECK(cells_reached(v) >= 9);
}
