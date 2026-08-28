#include <rc/test/rc_test.hpp>

#include "solution.hpp"

RC_TEST("a rate limiter takes at most one step") {
  RateLimiter limiter(1.0);
  RC_CHECK_NEAR(limiter.apply(10.0, 0.1), 0.1, 1e-9);
  RC_CHECK_NEAR(limiter.apply(10.0, 0.1), 0.2, 1e-9);
}

RC_TEST("a rate limiter settles exactly on the target") {
  RateLimiter limiter(1.0);
  for (int i = 0; i < 100; ++i) limiter.apply(0.5, 0.1);
  RC_CHECK_NEAR(limiter.current(), 0.5, 1e-9);
}

RC_TEST("a rate limiter works in both directions") {
  RateLimiter limiter(2.0, 1.0);
  RC_CHECK_NEAR(limiter.apply(-1.0, 0.1), 0.8, 1e-9);
}

RC_TEST("a time step of zero changes nothing") {
  RateLimiter limiter(1.0, 0.4);
  RC_CHECK_NEAR(limiter.apply(10.0, 0.0), 0.4, 1e-9);
  RC_CHECK_NEAR(limiter.apply(10.0, -1.0), 0.4, 1e-9);
}

RC_TEST("a watchdog that has never been fed is expired") {
  // This is the check that matters most. A watchdog which starts out trusting
  // hides the case where commands never arrived at all.
  const Watchdog dog(0.5, 0.0);
  RC_CHECK(dog.expired(0.0));
  RC_CHECK(dog.expired(100.0));
}

RC_TEST("a freshly fed watchdog is not expired") {
  Watchdog dog(0.5, 0.0);
  dog.feed(10.0);
  RC_CHECK(!dog.expired(10.0));
  RC_CHECK(!dog.expired(10.4));
}

RC_TEST("a watchdog expires once the timeout passes") {
  Watchdog dog(0.5, 0.0);
  dog.feed(10.0);
  RC_CHECK(!dog.expired(10.5));
  RC_CHECK(dog.expired(10.6));
}

RC_TEST("feeding again pushes the deadline out") {
  Watchdog dog(0.5, 0.0);
  dog.feed(10.0);
  dog.feed(10.4);
  RC_CHECK(!dog.expired(10.8));
  RC_CHECK(dog.expired(11.0));
}

RC_TEST("guard passes a fresh command through untouched") {
  Watchdog dog(0.5, 0.0);
  dog.feed(1.0);
  RC_CHECK_NEAR(dog.guard(0.9, 1.2), 0.9, 1e-9);
}

RC_TEST("guard forces the safe value once commands go stale") {
  Watchdog dog(0.5, 0.0);
  dog.feed(1.0);
  RC_CHECK_NEAR(dog.guard(0.9, 5.0), 0.0, 1e-9);
}

RC_TEST("the safe value is whatever the machine needs, not always zero") {
  // An arm that must hold position rather than go limp has a non zero safe
  // output, and the library must not decide that on the engineer's behalf.
  Watchdog holding_arm(0.2, 0.35);
  holding_arm.feed(0.0);
  RC_CHECK_NEAR(holding_arm.guard(0.9, 1.0), 0.35, 1e-9);
}

RC_TEST("a stalled controller is brought to the safe value") {
  // The command source stops after two seconds. Everything after that must be
  // safe, no matter what the last command was.
  Watchdog dog(0.5, 0.0);
  RateLimiter limiter(5.0);
  double output = 0.0;

  for (int tick = 0; tick < 400; ++tick) {
    const double now = tick * 0.01;
    if (now <= 2.0) dog.feed(now);
    output = limiter.apply(dog.guard(1.0, now), 0.01);
  }

  RC_CHECK_NEAR(output, 0.0, 1e-6);
}
