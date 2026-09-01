#include <rc/test/rc_test.hpp>

#include <cmath>
#include <string>
#include <vector>

#include "solution.hpp"

namespace {

// The one that is right. Every check must accept this, and a suite that rejects
// it is worse than no suite: it reports a bug in code that does not have one.
class Correct : public Limiter {
 public:
  double apply(double current, double target, double max_step) const override {
    const double remaining = target - current;
    if (std::fabs(remaining) <= max_step) return target;
    return current + (remaining > 0.0 ? max_step : -max_step);
  }
};

// Six that are wrong, each in one specific way, each passing every property
// except the one it breaks. A suite that catches all six had to have been
// written from what a rate limiter must do rather than from what this one does.

// Always takes a full step, so it steps past the target and back for ever.
class NeverArrives : public Limiter {
 public:
  double apply(double current, double target, double max_step) const override {
    return current + (target - current > 0.0 ? max_step : -max_step);
  }
};

// Does not limit at all.
class NoLimit : public Limiter {
 public:
  double apply(double, double target, double) const override { return target; }
};

// Only ever adds, so it runs away from any target below it.
class OnlyUpward : public Limiter {
 public:
  double apply(double current, double target, double max_step) const override {
    if (std::fabs(target - current) <= max_step) return target;
    return current + max_step;
  }
};

// Correct everywhere except when it has already arrived.
class RestlessAtTarget : public Limiter {
 public:
  double apply(double current, double target, double max_step) const override {
    if (current == target) return current + max_step;
    const double remaining = target - current;
    if (std::fabs(remaining) <= max_step) return target;
    return current + (remaining > 0.0 ? max_step : -max_step);
  }
};

// Moves even when asked to move by nothing.
class IgnoresZeroStep : public Limiter {
 public:
  double apply(double current, double target, double max_step) const override {
    const double step = max_step <= 0.0 ? 0.01 : max_step;
    const double remaining = target - current;
    if (std::fabs(remaining) <= step) return target;
    return current + (remaining > 0.0 ? step : -step);
  }
};

// Wrong only when the distance is exactly one step, which is the case random
// sampling almost never produces and a chosen boundary always does.
class WrongAtExactlyOneStep : public Limiter {
 public:
  double apply(double current, double target, double max_step) const override {
    const double remaining = target - current;
    if (std::fabs(std::fabs(remaining) - max_step) < 1e-12) {
      return current + (remaining > 0.0 ? max_step * 1.5 : -max_step * 1.5);
    }
    if (std::fabs(remaining) <= max_step) return target;
    return current + (remaining > 0.0 ? max_step : -max_step);
  }
};

struct Broken {
  const char* name;
  const Limiter* limiter;
};

}  // namespace

RC_TEST("the checks accept an implementation that is right") {
  // A suite that rejects correct code is worse than no suite at all, because it
  // reports a bug that does not exist and the next real one is not believed.
  const Correct correct;
  RC_CHECK(checks_pass(correct));
}

RC_TEST("the checks reject a limiter that steps past the target for ever") {
  const NeverArrives broken;
  RC_CHECK(!checks_pass(broken));
}

RC_TEST("the checks reject a limiter that does not limit") {
  const NoLimit broken;
  RC_CHECK(!checks_pass(broken));
}

RC_TEST("the checks reject a limiter that only ever moves upward") {
  // The one a suite written entirely with the target above the current value
  // will miss, which is most suites written without thinking about it.
  const OnlyUpward broken;
  RC_CHECK(!checks_pass(broken));
}

RC_TEST("the checks reject a limiter that will not stay where it arrived") {
  const RestlessAtTarget broken;
  RC_CHECK(!checks_pass(broken));
}

RC_TEST("the checks reject a limiter that moves when asked to move by nothing") {
  const IgnoresZeroStep broken;
  RC_CHECK(!checks_pass(broken));
}

RC_TEST("the checks reject a limiter that is wrong at exactly one step away") {
  // The boundary between arriving and stepping. Sampling at random lands on it
  // with probability nearly zero; choosing it deliberately lands on it every
  // time, which is the difference between the two ways of writing a test.
  const WrongAtExactlyOneStep broken;
  RC_CHECK(!checks_pass(broken));
}

RC_TEST("every broken implementation is caught, and the correct one is not") {
  // The summary that says what the suite is for. Six ways to be wrong, one way
  // to be right, and a set of checks has to separate them.
  const Correct correct;
  const NeverArrives never_arrives;
  const NoLimit no_limit;
  const OnlyUpward only_upward;
  const RestlessAtTarget restless;
  const IgnoresZeroStep ignores_zero;
  const WrongAtExactlyOneStep wrong_at_boundary;

  const std::vector<Broken> broken{
      {"steps past the target for ever", &never_arrives},
      {"does not limit at all", &no_limit},
      {"only ever moves upward", &only_upward},
      {"will not stay where it arrived", &restless},
      {"moves when asked to move by nothing", &ignores_zero},
      {"wrong at exactly one step away", &wrong_at_boundary},
  };

  RC_REQUIRE(checks_pass(correct));
  for (const Broken& entry : broken) {
    if (!checks_pass(*entry.limiter)) continue;
    RC_CHECK_EQ(std::string("uncaught: ") + entry.name, std::string("caught"));
  }
}
