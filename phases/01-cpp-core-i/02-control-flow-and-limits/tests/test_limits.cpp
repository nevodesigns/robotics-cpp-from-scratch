#include <rc/test/rc_test.hpp>

#include <cmath>
#include <iostream>
#include <sstream>
#include <string>

#include "solution.hpp"

namespace {

int g_calls = 0;
int counted_value() {
  ++g_calls;
  return 7;
}

}  // namespace

RC_TEST("clamp leaves a value inside the range alone") {
  RC_CHECK_NEAR(clamp(0.5, 0.0, 1.0), 0.5, 1e-9);
}

RC_TEST("clamp pulls values back to the edges") {
  RC_CHECK_NEAR(clamp(-3.0, 0.0, 1.0), 0.0, 1e-9);
  RC_CHECK_NEAR(clamp(9.0, 0.0, 1.0), 1.0, 1e-9);
}

RC_TEST("clamp works with negative ranges") {
  RC_CHECK_NEAR(clamp(-5.0, -1.0, 1.0), -1.0, 1e-9);
  RC_CHECK_NEAR(clamp(0.0, -1.0, -0.5), -0.5, 1e-9);
}

RC_TEST("a large change is limited to one step") {
  RC_CHECK_NEAR(rate_limit(0.0, 1.0, 0.1), 0.1, 1e-9);
  RC_CHECK_NEAR(rate_limit(0.0, -1.0, 0.1), -0.1, 1e-9);
}

RC_TEST("a small change arrives exactly, without overshooting") {
  RC_CHECK_NEAR(rate_limit(0.95, 1.0, 0.1), 1.0, 1e-9);
  RC_CHECK_NEAR(rate_limit(1.0, 1.0, 0.1), 1.0, 1e-9);
}

RC_TEST("repeated limiting arrives and then stays put") {
  double value = 0.0;
  for (int i = 0; i < 100; ++i) value = rate_limit(value, 1.0, 0.1);
  RC_CHECK_NEAR(value, 1.0, 1e-9);
}

RC_TEST("counting the steps to full speed") {
  RC_CHECK_EQ(steps_to_reach(0.0, 1.0, 0.1), 10);
  RC_CHECK_EQ(steps_to_reach(1.0, 1.0, 0.1), 0);
  RC_CHECK_EQ(steps_to_reach(0.0, -0.5, 0.1), 5);
}

RC_TEST("a step size of zero reports that it never arrives") {
  RC_CHECK_EQ(steps_to_reach(0.0, 1.0, 0.0), -1);
  RC_CHECK_EQ(steps_to_reach(0.0, 1.0, -0.1), -1);
}

RC_TEST("the conditional operator picks one side, binds loosely, and gives both one type") {
  // It is a clamp in one expression, so the learner's clamp is held to it.
  // An all zero stub would pass a test that only measured the operator.
  int agreed = 0;
  int total = 0;
  for (int i = -20; i <= 20; ++i) {
    const double value = i / 10.0;
    const double nested = value < 0.0 ? 0.0 : (value > 1.0 ? 1.0 : value);
    ++total;
    if (std::fabs(clamp(value, 0.0, 1.0) - nested) < 1e-12) ++agreed;
  }
  RC_CHECK_EQ(agreed, total);

  // Only one side is evaluated.
  const bool flag = true;
  g_calls = 0;
  const int chosen = flag ? counted_value() : counted_value();
  RC_CHECK_EQ(chosen, 7);
  RC_CHECK_EQ(g_calls, 1);

  const int steps = 0;
  const int per_step = steps == 0 ? 0 : 100 / steps;
  RC_CHECK_EQ(per_step, 0);

  // It binds more loosely than <<, so the condition is everything to its left.
  std::ostringstream loose;
  const char* thrown_away = (loose << "ok: " << flag) ? "yes" : "no";
  std::ostringstream tight;
  tight << "ok: " << (flag ? "yes" : "no");

  std::cout << "  out << \"ok: \" << flag ? \"yes\" : \"no\"      the stream held \"" << loose.str()
            << "\"\n";
  std::cout << "  out << \"ok: \" << (flag ? \"yes\" : \"no\")    the stream held \"" << tight.str()
            << "\"\n\n";
  RC_CHECK(std::string(thrown_away) == "yes");
  RC_CHECK_EQ(loose.str(), std::string("ok: 1"));
  RC_CHECK_EQ(tight.str(), std::string("ok: yes"));

  // Both sides become one type.
  std::cout << "  sizeof(flag ? 1 : 2.5) is " << sizeof(flag ? 1 : 2.5) << ", sizeof(1) is "
            << sizeof(1) << "\n\n";
  RC_CHECK_EQ(sizeof(flag ? 1 : 2.5), sizeof(double));
}
