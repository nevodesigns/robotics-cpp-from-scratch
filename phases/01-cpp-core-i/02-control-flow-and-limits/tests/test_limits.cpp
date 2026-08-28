#include <rc/test/rc_test.hpp>

#include "solution.hpp"

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
