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
    {"hidden", 8.0},
    {"params", 33.0},
    {"bad_rel", 0.12286002077844321},
    {"lr", 0.1},
    {"err0", 0.030142608108954395},
    {"err1", 0.01838400038193652},
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


RC_TEST("a network made from a seed has the weights it should, and the same seed gives the same network") {
  const Mlp m = make_mlp(8, 1.0, 4);
  rec("hidden", m.hidden, 0);
  rec("params", param_count(m), 0);
  RC_CHECK(param_count(m) == 33);
  RC_CHECK(m.w1.size() == 16u && m.b1.size() == 8u && m.w2.size() == 8u);
  const Mlp a = make_mlp(8, 1.0, 4), b = make_mlp(8, 1.0, 5);
  RC_CHECK(a.w1[3] == m.w1[3]);
  RC_CHECK(b.w1[3] != m.w1[3]);
  for (double w : m.w1) RC_CHECK(std::fabs(w) <= 1.0);
  // with all output weights zero the output is the output bias, whatever the input
  Mlp z = m;
  for (double& w : z.w2) w = 0.0;
  z.b2 = 0.25;
  const double x[2] = {3.0, -2.0};
  RC_CHECK(mlp_forward(z, x, nullptr) == 0.25);
}

RC_TEST("the backpropagated gradient agrees with finite differences for every weight") {
  const Mlp m = make_mlp(8, 1.0, 4);
  const double x[2] = {0.7, -0.4};
  const double worst = numeric_check(m, x, 0.3, 1e-5);
  RC_CHECK(worst < 1e-6);
  // at other inputs and targets as well
  const double x2[2] = {-1.5, 2.0};
  RC_CHECK(numeric_check(m, x2, -0.8, 1e-5) < 1e-6);
  // the gradient of the output bias is the error itself
  MlpGrad g;
  const double e = mlp_gradient(m, x, 0.3, g);
  RC_CHECK(std::fabs(g.b2 - e) < 1e-15);
}

RC_TEST("a gradient with one mistake in it is caught by the check, and a small step against the right gradient lowers the error") {
  const Mlp m = make_mlp(8, 1.0, 4);
  const double x[2] = {0.7, -0.4};
  // the mistake: the derivative of tanh left out of the hidden layer's gradient
  MlpGrad g;
  const double e = mlp_gradient(m, x, 0.3, g);
  double bad = 0.0;
  std::vector<double> z(8, 0.0);
  mlp_forward(m, x, &z);
  for (int i = 0; i < 8; ++i) {
    const double wrong = e * m.w2[static_cast<std::size_t>(i)] * x[0];
    const double right = g.w1[static_cast<std::size_t>(2 * i)];
    const double rel = std::fabs(wrong - right) / (std::fabs(wrong) + std::fabs(right) + 1e-12);
    if (rel > bad) bad = rel;
  }
  rec("bad_rel", bad, 3);
  RC_CHECK(bad > 1e-3);
  // one step of gradient descent on one sample
  const double lr = 0.1;
  Mlp s = m;
  for (std::size_t k = 0; k < s.w1.size(); ++k) s.w1[k] -= lr * g.w1[k];
  for (std::size_t k = 0; k < s.b1.size(); ++k) s.b1[k] -= lr * g.b1[k];
  for (std::size_t k = 0; k < s.w2.size(); ++k) s.w2[k] -= lr * g.w2[k];
  s.b2 -= lr * g.b2;
  const double e1 = mlp_forward(s, x, nullptr) - 0.3;
  rec("lr", lr, 1);
  rec("err0", 0.5 * e * e, 5);
  rec("err1", 0.5 * e1 * e1, 5);
  RC_CHECK(0.5 * e1 * e1 < 0.5 * e * e);
}
