#include <rc/test/rc_test.hpp>

#include <cmath>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

#include "solution.hpp"

namespace {

struct Variant {
  const char* name;
  double kp;
  double ki;
  double tau;
  double dt;
};

// Changes that must not matter: a finer or coarser time step, and a plant a little faster or slower than assumed.
const Variant kHarmless[] = {{"time step halved", 2.0, 3.0, 0.5, 0.0005},
                             {"time step doubled", 2.0, 3.0, 0.5, 0.002},
                             {"plant 5 percent slower", 2.0, 3.0, 0.525, 0.001},
                             {"plant 5 percent faster", 2.0, 3.0, 0.475, 0.001},
                             {"plant 10 percent slower", 2.0, 3.0, 0.55, 0.001},
                             {"plant 10 percent faster", 2.0, 3.0, 0.45, 0.001}};

// Changes that are bugs.
const Variant kHarmful[] = {{"integral term lost", 2.0, 0.0, 0.5, 0.001},
                            {"gains too low", 0.2, 0.1, 0.5, 0.001},
                            {"integral too strong", 0.5, 10.0, 0.5, 0.001},
                            {"gains too high", 6.0, 60.0, 0.5, 0.001},
                            {"sign flipped", -2.0, -3.0, 0.5, 0.001}};

Metrics run(const Variant& v) { return measure(simulate(v.kp, v.ki, v.tau, v.dt, 10.0)); }

Response synthetic(const std::vector<double>& y, double dt = 0.1) {
  Response r;
  r.y = y;
  r.dt = dt;
  r.target = 1.0;
  return r;
}

}  // namespace

RC_TEST("overshoot, settling time and steady error are read off a response") {
  // Rises to 1.2 and comes back to 1: an overshoot of 20 percent.
  const Metrics a = measure(synthetic({0.5, 1.0, 1.2, 1.1, 1.03, 1.01, 1.0, 1.0, 1.0, 1.0}));
  RC_CHECK_NEAR(a.overshoot_percent, 20.0, 1e-9);
  RC_CHECK_NEAR(a.settling_time, 0.5, 1e-9);       // last more than 2 percent out at the fifth sample, 0.5 s
  RC_CHECK_NEAR(a.steady_error, 0.0, 1e-9);
  RC_CHECK(!a.diverged);

  // Never reaches the target: no overshoot, and the steady error is what is left.
  const Metrics b = measure(synthetic({0.2, 0.4, 0.6, 0.65, 0.66, 0.66, 0.66, 0.66, 0.66, 0.66}));
  RC_CHECK_NEAR(b.overshoot_percent, 0.0, 1e-9);
  RC_CHECK_NEAR(b.steady_error, 0.34, 1e-9);
  RC_CHECK_NEAR(b.settling_time, 1.0, 1e-9);

  // Already there: settles at once.
  const Metrics c = measure(synthetic({1.0, 1.0, 1.0, 1.0}));
  RC_CHECK_NEAR(c.settling_time, 0.0, 1e-12);
  RC_CHECK_NEAR(c.overshoot_percent, 0.0, 1e-12);

  // Runs away, and not a number.
  RC_CHECK(measure(synthetic({1.0, 1e9, -1e12, 1e20})).diverged);
  RC_CHECK(measure(synthetic({1.0, std::nan(""), 1.0, 1.0})).diverged);
  RC_CHECK(measure(synthetic({})).diverged);
}

RC_TEST("a response meets a specification only if every number is inside it, and a runaway never does") {
  Spec spec;
  Metrics good;
  good.overshoot_percent = 5.0;
  good.settling_time = 2.0;
  good.steady_error = 0.001;
  RC_CHECK(meets(spec, good));

  Metrics m = good;
  m.overshoot_percent = 10.5;
  RC_CHECK(!meets(spec, m));
  m = good;
  m.settling_time = 3.5;
  RC_CHECK(!meets(spec, m));
  m = good;
  m.steady_error = 0.05;
  RC_CHECK(!meets(spec, m));
  m = good;
  m.diverged = true;
  RC_CHECK(!meets(spec, m));
}

RC_TEST("a test that pins an exact value breaks on changes that do not matter") {
  // The value of the response at one second, written down from the first version that worked.
  const Response base = simulate(2.0, 3.0, 0.5, 0.001, 10.0);
  const double pinned = base.y[999];
  std::cout << "  pinned value at one second: " << std::fixed << std::setprecision(6) << pinned << "\n";

  int broken = 0;
  for (const Variant& v : kHarmless) {
    const Response r = simulate(v.kp, v.ki, v.tau, v.dt, 10.0);
    const std::size_t index = static_cast<std::size_t>(1.0 / v.dt) - 1;
    const bool same = std::fabs(r.y[index] - pinned) < 1e-6;
    std::cout << "    " << std::left << std::setw(26) << v.name << " value " << std::right << std::setprecision(6) << r.y[index]
              << (same ? "  test passes" : "  test fails") << "\n";
    if (!same) ++broken;
  }
  std::cout << "  " << broken << " of " << sizeof(kHarmless) / sizeof(kHarmless[0]) << " harmless changes broke the exact test\n\n";
  RC_CHECK(broken >= 5);

  // The behaviour of the very response that was pinned is fine by the specification, which is what should have been written down.
  RC_CHECK(meets(Spec(), measure(base)));
}

RC_TEST("a test of the behaviour passes every harmless change and fails every real bug") {
  Spec spec;
  std::cout << "  variant                      overshoot %   settling s   steady error   verdict\n";
  int harmless_pass = 0;
  for (const Variant& v : kHarmless) {
    const Metrics m = run(v);
    const bool ok = meets(spec, m);
    std::cout << "  " << std::left << std::setw(26) << v.name << std::right << std::fixed << std::setprecision(2) << std::setw(12)
              << m.overshoot_percent << std::setw(13) << m.settling_time << std::setprecision(4) << std::setw(15) << m.steady_error
              << "   " << (ok ? "meets" : "FAILS") << "\n";
    if (ok) ++harmless_pass;
  }
  int harmful_caught = 0;
  for (const Variant& v : kHarmful) {
    const Metrics m = run(v);
    const bool ok = meets(spec, m);
    std::cout << "  " << std::left << std::setw(26) << v.name << std::right << std::fixed << std::setprecision(2) << std::setw(12)
              << m.overshoot_percent << std::setw(13) << m.settling_time << std::setprecision(4) << std::setw(15)
              << (m.diverged ? 999.0 : m.steady_error) << "   " << (ok ? "meets" : "FAILS") << "\n";
    if (!ok) ++harmful_caught;
  }
  std::cout << "\n  harmless changes passed: " << harmless_pass << " of " << sizeof(kHarmless) / sizeof(kHarmless[0])
            << ", bugs caught: " << harmful_caught << " of " << sizeof(kHarmful) / sizeof(kHarmful[0]) << "\n\n";
  RC_CHECK_EQ(harmless_pass, 6);
  RC_CHECK_EQ(harmful_caught, 5);
  RC_CHECK(meets(spec, measure(simulate(2.0, 3.0, 0.5, 0.001, 10.0))));
}

RC_TEST("the numbers of the specification can be tightened until it fails a change it should not") {
  // A specification too tight is the pinned value again: with a settling time of 1.8 s, a plant ten percent slower fails it.
  Spec tight;
  tight.max_settling_time = 1.8;
  int failed = 0;
  for (const Variant& v : kHarmless) {
    if (!meets(tight, run(v))) ++failed;
  }
  std::cout << "  with the settling time cut to 1.8 s, " << failed << " of 6 harmless changes fail\n\n";
  RC_CHECK(failed >= 2);
  RC_CHECK(failed < 6);
}
