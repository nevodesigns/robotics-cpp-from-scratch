#include <rc/test/rc_test.hpp>

#include <cmath>
#include <iomanip>
#include <iostream>
#include <vector>

#include "solution.hpp"

namespace {

// A mass on a spring. Its acceleration depends only on where it is, its energy
// is exactly constant, and its motion has a closed form, so every number below
// is compared against the truth rather than against another simulation.
constexpr double kOmega = 2.0;   // radians per second

double spring(double position) { return -kOmega * kOmega * position; }

double energy(const Motion& state) {
  return 0.5 * state.velocity * state.velocity +
         0.5 * kOmega * kOmega * state.position * state.position;
}

Motion exact_at(double seconds, double amplitude) {
  return Motion{amplitude * std::cos(kOmega * seconds),
                -amplitude * kOmega * std::sin(kOmega * seconds)};
}

enum class Method { euler, semi_implicit, rk4 };

Motion advance(Method method, const Motion& state, double dt) {
  switch (method) {
    case Method::euler: return euler_step(state, spring, dt);
    case Method::semi_implicit: return semi_implicit_step(state, spring, dt);
    case Method::rk4: return rk4_step(state, spring, dt);
  }
  return state;
}

Motion run(Method method, const Motion& start, double dt, long steps) {
  Motion state = start;
  for (long i = 0; i < steps; ++i) state = advance(method, state, dt);
  return state;
}

// The error in position after one second, from a given step.
double error_after_one_second(Method method, double dt) {
  const Motion start{1.0, 0.0};
  const long steps = static_cast<long>(1.0 / dt + 0.5);
  const Motion got = run(method, start, dt, steps);
  return std::fabs(got.position - exact_at(1.0, 1.0).position);
}

}  // namespace

RC_TEST("one step of each, against a step taken by hand") {
  const Motion start{1.0, 0.0};
  const double dt = 0.1;

  // Explicit Euler advances both halves from where the step began.
  const Motion e = euler_step(start, spring, dt);
  RC_CHECK_NEAR(e.position, 1.0 + 0.0 * dt, 1e-15);
  RC_CHECK_NEAR(e.velocity, 0.0 + spring(1.0) * dt, 1e-15);

  // Semi-implicit advances the velocity first and then uses it.
  const Motion s = semi_implicit_step(start, spring, dt);
  RC_CHECK_NEAR(s.velocity, 0.0 + spring(1.0) * dt, 1e-15);
  RC_CHECK_NEAR(s.position, 1.0 + s.velocity * dt, 1e-15);

  // The two differ only in the position, and only by the velocity's own change
  // over the step. One line, and the whole of this lesson.
  RC_CHECK_EQ(e.velocity, s.velocity);
  RC_CHECK_NEAR(e.position - s.position, -spring(1.0) * dt * dt, 1e-15);

  // And Runge Kutta lands much closer to the truth for the same one step.
  const Motion r = rk4_step(start, spring, dt);
  const Motion truth = exact_at(dt, 1.0);
  RC_CHECK(std::fabs(r.position - truth.position) <
           std::fabs(s.position - truth.position) * 0.01);

  // A step of nothing changes nothing.
  const Motion still = rk4_step(start, spring, 0.0);
  RC_CHECK_EQ(still.position, start.position);
  RC_CHECK_EQ(still.velocity, start.velocity);
}

RC_TEST("explicit Euler does not merely have error, it adds energy") {
  const Motion start{1.0, 0.0};
  const double dt = 0.01;
  const double at_the_start = energy(start);

  std::cout << "\n    a mass on a spring at 2 rad/s, dt = 0.01 s, energy as a\n";
  std::cout << "    fraction of where it started\n\n";
  std::cout << "    " << std::right << std::setw(10) << "seconds" << std::setw(18)
            << "euler" << std::setw(18) << "semi-implicit" << std::setw(14)
            << "rk4" << "\n";

  Motion a = start, b = start, c = start;
  double worst_semi = 0.0, worst_rk4 = 0.0;
  for (long i = 1; i <= 60000; ++i) {
    a = advance(Method::euler, a, dt);
    b = advance(Method::semi_implicit, b, dt);
    c = advance(Method::rk4, c, dt);

    const double semi_off = std::fabs(energy(b) / at_the_start - 1.0);
    const double rk4_off = std::fabs(energy(c) / at_the_start - 1.0);
    if (semi_off > worst_semi) worst_semi = semi_off;
    if (rk4_off > worst_rk4) worst_rk4 = rk4_off;

    if (i % 10000 == 0)
      std::cout << "    " << std::right << std::setw(10) << i * dt << std::fixed
                << std::setprecision(4) << std::setw(18) << energy(a) / at_the_start
                << std::setprecision(6) << std::setw(18) << energy(b) / at_the_start
                << std::setw(14) << energy(c) / at_the_start << "\n";
  }

  std::cout << "\n    ten simulated minutes. The first column is a spring that\n";
  std::cout << "    nobody pushed, holding twenty six billion times the energy\n";
  std::cout << "    it was given\n";

  // Explicit Euler gains energy without bound.
  RC_CHECK(energy(a) / at_the_start > 1e9);

  // Semi-implicit stays inside a small band, for ever. It never converges to
  // the right energy and it never leaves.
  RC_CHECK(worst_semi < 0.02);

  // Runge Kutta at this step holds it to within a millionth.
  RC_CHECK(worst_rk4 < 1e-5);
}

RC_TEST("the more accurate of the two is the one that blows up") {
  const double dt = 0.02;

  std::cout << "\n    error in position after one second, at dt = 0.02\n\n";
  std::cout << "    " << std::left << std::setw(20) << "method" << std::right
            << std::setw(14) << "error" << "\n";
  const double e = error_after_one_second(Method::euler, dt);
  const double s = error_after_one_second(Method::semi_implicit, dt);
  std::cout << "    " << std::left << std::setw(20) << "explicit euler"
            << std::right << std::scientific << std::setprecision(3) << std::setw(14)
            << e << "\n";
  std::cout << "    " << std::left << std::setw(20) << "semi-implicit"
            << std::right << std::setw(14) << s << "\n" << std::defaultfloat;

  std::cout << "\n    over one second the explicit method is the closer of the\n";
  std::cout << "    two, and over ten minutes it has gained twenty six billion\n";
  std::cout << "    times its energy while the other has not moved. Accuracy\n";
  std::cout << "    and stability are different properties and a short test\n";
  std::cout << "    only measures the first\n";

  // Explicit Euler is genuinely no worse over a short run, and is slightly
  // better here, which is exactly why a one second test chooses the wrong one.
  RC_CHECK(e < s);
  RC_CHECK(e > s * 0.5);
}

RC_TEST("halving the step, and what each method does with it") {
  std::cout << "\n    error in position after one second\n\n";
  std::cout << "    " << std::right << std::setw(10) << "dt" << std::setw(13)
            << "euler" << std::setw(8) << "ratio" << std::setw(13) << "semi-impl"
            << std::setw(8) << "ratio" << std::setw(13) << "rk4" << std::setw(8)
            << "ratio" << "\n";

  double previous[3] = {0.0, 0.0, 0.0};
  std::vector<double> rk4_ratios;
  double last_semi_ratio = 0.0;
  for (const double dt : {0.02, 0.01, 0.005, 0.0025, 0.00125}) {
    const double values[3] = {error_after_one_second(Method::euler, dt),
                              error_after_one_second(Method::semi_implicit, dt),
                              error_after_one_second(Method::rk4, dt)};
    std::cout << "    " << std::right << std::fixed << std::setprecision(5)
              << std::setw(10) << dt;
    for (int i = 0; i < 3; ++i) {
      std::cout << std::scientific << std::setprecision(3) << std::setw(13) << values[i]
                << std::fixed << std::setprecision(2) << std::setw(8)
                << (previous[i] > 0.0 ? previous[i] / values[i] : 0.0);
    }
    std::cout << "\n";
    if (previous[2] > 0.0) rk4_ratios.push_back(previous[2] / values[2]);
    if (previous[1] > 0.0) last_semi_ratio = previous[1] / values[1];
    for (int i = 0; i < 3; ++i) previous[i] = values[i];
  }
  std::cout << std::defaultfloat;

  std::cout << "\n    both Euler methods are first order: half the step, half\n";
  std::cout << "    the error. Runge Kutta is fourth: half the step, a\n";
  std::cout << "    sixteenth of the error\n";

  // First order, to two decimal places.
  RC_CHECK_NEAR(last_semi_ratio, 2.0, 0.05);

  // Fourth order, to within a few percent, on every halving.
  RC_REQUIRE(!rk4_ratios.empty());
  for (const double ratio : rk4_ratios) RC_CHECK_NEAR(ratio, 16.0, 1.0);
}

RC_TEST("compared at equal work rather than at equal step") {
  // Runge Kutta calls the acceleration four times per step, so a fair
  // comparison gives Euler four times as many steps.
  std::cout << "\n    the same number of calls to the acceleration\n\n";
  std::cout << "    " << std::left << std::setw(30) << "" << std::right
            << std::setw(14) << "error" << "\n";

  for (const double dt : {0.02, 0.01}) {
    const double euler_fine = error_after_one_second(Method::euler, dt / 4.0);
    const double rk4_coarse = error_after_one_second(Method::rk4, dt);
    std::cout << "    " << std::left << std::setw(30)
              << ("euler at " + std::to_string(dt / 4.0).substr(0, 7)) << std::right
              << std::scientific << std::setprecision(3) << std::setw(14) << euler_fine
              << "\n";
    std::cout << "    " << std::left << std::setw(30)
              << ("rk4 at " + std::to_string(dt).substr(0, 7)) << std::right
              << std::setw(14) << rk4_coarse << "\n" << std::defaultfloat;

    // Five orders of magnitude, for the same four calls.
    RC_CHECK(rk4_coarse < euler_fine * 1e-4);
  }

  std::cout << "\n    a method that costs four times as much per step is not\n";
  std::cout << "    four times as expensive to use, and comparing at equal\n";
  std::cout << "    step is comparing the wrong thing\n";
}

RC_TEST("a reference made by taking more steps stops improving") {
  const Motion start{1.0, 0.0};
  const double truth = exact_at(1.0, 1.0).position;

  std::cout << "\n    a reference solution built by taking more Runge Kutta\n";
  std::cout << "    steps, against the closed form answer\n\n";
  std::cout << "    " << std::right << std::setw(12) << "steps" << std::setw(14)
            << "step size" << std::setw(16) << "error" << "\n";

  double best = -1.0;
  long best_steps = 0;
  double at_ten_million = 0.0;
  for (const long steps : {100L, 1000L, 10000L, 100000L, 1000000L, 10000000L}) {
    const double dt = 1.0 / static_cast<double>(steps);
    const double error = std::fabs(run(Method::rk4, start, dt, steps).position - truth);
    if (best < 0.0 || error < best) { best = error; best_steps = steps; }
    if (steps == 10000000L) at_ten_million = error;
    std::cout << "    " << std::right << std::setw(12) << steps << std::scientific
              << std::setprecision(2) << std::setw(14) << dt << std::setprecision(3)
              << std::setw(16) << error << "\n" << std::defaultfloat;
  }

  std::cout << "\n    the best reference here took ten thousand steps. A\n";
  std::cout << "    thousand times more of them was worse, because every step\n";
  std::cout << "    rounds and a million roundings are larger than the\n";
  std::cout << "    truncation they were spent removing\n";
  std::cout << "\n    which is the same V as the derivative in lesson 06-05,\n";
  std::cout << "    from the same two errors pulling opposite ways\n";

  // There is a floor, and going past it makes things worse rather than better.
  RC_CHECK(best_steps <= 100000L);
  RC_CHECK(at_ten_million > best * 5.0);

  // So a simulation compared against a finer version of itself cannot resolve
  // an error smaller than this, and the closed form is what makes the rest of
  // this suite meaningful.
  RC_CHECK(best > 0.0);
  RC_CHECK(best < 1e-14);
}
