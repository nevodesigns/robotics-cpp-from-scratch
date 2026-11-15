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
    {"score0", 36.85},
    {"n", 20.0},
    {"step", 2.0},
    {"limit", 500.0},
    {"score10", 64.0},
    {"score50", 207.7},
    {"score300", 500.0},
    {"k0", -0.8268993893655838},
    {"k1", 4.974004923261486},
    {"k2", 9.060860062895543},
    {"k3", 19.940077069622767},
    {"runs", 6000.0},
    {"n_test", 100.0},
    {"test300", 497.93},
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


RC_TEST("the pole on a cart behaves as a pole should: balanced it stays, tipped it falls, pushed the cart moves") {
  const CartModel m;
  CartState up;
  for (int i = 0; i < 100; ++i) cart_step(m, up, 0.0);
  RC_CHECK(up.angle == 0.0 && up.x == 0.0);
  CartState tipped;
  tipped.angle = 0.05;
  int steps = 0;
  while (std::fabs(tipped.angle) <= m.max_angle && steps < 1000) {
    cart_step(m, tipped, 0.0);
    ++steps;
  }
  RC_CHECK(steps < 100 && tipped.angle > 0.0);
  CartState pushed;
  cart_step(m, pushed, 50.0);   // clipped to 10 N
  cart_step(m, pushed, 0.0);
  RC_CHECK(pushed.v > 0.0 && std::fabs(pushed.v - 10.0 / 1.1 * m.dt) < 0.02);
}

RC_TEST("random search finds gains that balance the pole, from gains of zero that let it fall") {
  const CartModel m;
  const int n = 20, limit = 500, tries = 300;
  const double step = 2.0;
  const double zero[4] = {0.0, 0.0, 0.0, 0.0};
  double K[4];
  std::vector<double> h;
  random_search(m, tries, step, n, 11, 5, limit, K, &h);
  rec("score0", balance_score(m, zero, n, 11, limit), 1);
  rec("n", n, 0);
  rec("step", step, 1);
  rec("limit", limit, 0);
  rec("score10", h[9], 1);
  rec("score50", h[49], 1);
  rec("score300", h[299], 1);
  rec("k0", K[0], 2);
  rec("k1", K[1], 2);
  rec("k2", K[2], 2);
  rec("k3", K[3], 2);
  rec("runs", tries * n, 0);
  RC_CHECK(h.size() == 300u);
  for (std::size_t i = 1; i < h.size(); ++i) RC_CHECK(h[i] >= h[i - 1]);
  RC_CHECK(h[299] > 400.0);
  RC_CHECK(h[299] > 5.0 * balance_score(m, zero, n, 11, limit));
}

RC_TEST("the gains it found balance from starts the search never saw") {
  const CartModel m;
  double K[4];
  random_search(m, 300, 2.0, 20, 11, 5, 500, K, nullptr);
  const int n_test = 100;
  const double test = balance_score(m, K, n_test, 99, 500);
  rec("n_test", n_test, 0);
  rec("test300", test, 1);
  RC_CHECK(test > 400.0);
}
