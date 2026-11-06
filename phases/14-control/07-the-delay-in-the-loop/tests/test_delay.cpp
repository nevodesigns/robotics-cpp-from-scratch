#include <rc/test/rc_test.hpp>

#include <cmath>
#include <complex>
#include <iomanip>
#include <iostream>
#include <limits>

#include "solution.hpp"

namespace {

const double kPi = 3.141592653589793;
const double kInertia = 0.5;      // kg m^2, a pure inertia: no friction to help
const double kStep = 0.001;       // the loop runs at 1 kHz

// The loop gain of a PD controller on a pure inertia with a delay, evaluated as a
// complex number at a frequency. This is the definition, and nothing in the code
// under test is used to compute it.
std::complex<double> loop_gain(double kp, double kd, double inertia, double delay, double w) {
  const std::complex<double> s(0.0, w);
  return (kp + kd * s) / (inertia * s * s) * std::exp(-s * delay);
}

// The frequency where the loop gain has size one, found by searching, since the size
// only falls as the frequency rises.
double crossover_by_search(double kp, double kd, double inertia) {
  double lo = 1e-6;
  double hi = 1e9;
  for (int i = 0; i < 200; ++i) {
    const double mid = std::sqrt(lo * hi);
    if (std::abs(loop_gain(kp, kd, inertia, 0.0, mid)) > 1.0) lo = mid;
    else hi = mid;
  }
  return std::sqrt(lo * hi);
}

// Whether the step response of the loop, with the command arriving `delay` late,
// settles. A time domain simulation, at one millisecond, with the delay a line of
// samples: the truth the margin is a prediction of.
bool settles(double kp, double kd, double delay) {
  const std::size_t samples = static_cast<std::size_t>(std::lround(delay / kStep));
  DelayLine line(samples);
  double theta = 0.0;
  double omega = 0.0;
  const double target = 1.0;
  double late_worst = 0.0;

  for (int i = 0; i < 40000; ++i) {
    const double command = kp * (target - theta) - kd * omega;
    const double applied = line.push(command);
    omega += applied / kInertia * kStep;
    theta += omega * kStep;
    if (std::fabs(theta) > 1e3) return false;
    if (i >= 30000) late_worst = std::fmax(late_worst, std::fabs(target - theta));
  }
  return late_worst < 1e-2;
}

// The largest gain at which it settles, by the same halving on a log scale.
double simulated_limit(double delay, double zeta) {
  const auto stable = [&](double kp) { return settles(kp, 2.0 * zeta * std::sqrt(kp * kInertia), delay); };
  double lo = 1.0;
  double hi = 1e7;
  if (!stable(lo)) return 0.0;
  while (hi / lo > 1.01) {
    const double mid = std::sqrt(lo * hi);
    if (stable(mid)) lo = mid;
    else hi = mid;
  }
  return lo;
}

}  // namespace

RC_TEST("a delay line returns what went in some samples ago, and is a wire for none") {
  DelayLine three(3);
  RC_CHECK_EQ(three.samples(), static_cast<std::size_t>(3));
  const double in[] = {1, 2, 3, 4, 5, 6, 7};
  const double expected[] = {0, 0, 0, 1, 2, 3, 4};
  for (int i = 0; i < 7; ++i) RC_CHECK_NEAR(three.push(in[i]), expected[i], 1e-15);

  // Until it has filled, what comes out is the initial value, not something made up.
  DelayLine filled(2, 9.5);
  RC_CHECK_NEAR(filled.push(1.0), 9.5, 1e-15);
  RC_CHECK_NEAR(filled.push(2.0), 9.5, 1e-15);
  RC_CHECK_NEAR(filled.push(3.0), 1.0, 1e-15);

  DelayLine wire(0);
  RC_CHECK_EQ(wire.samples(), static_cast<std::size_t>(0));
  RC_CHECK_NEAR(wire.push(4.25), 4.25, 1e-15);
  RC_CHECK_NEAR(wire.push(-1.0), -1.0, 1e-15);

  // One sample is a one step lag, however many times round the ring it has gone.
  DelayLine one(1);
  double last = 0.0;
  for (int i = 1; i <= 1000; ++i) {
    const double out = one.push(static_cast<double>(i));
    if (i > 1) RC_CHECK_NEAR(out, last, 1e-15);
    last = static_cast<double>(i);
  }
}

RC_TEST("a delay is a phase that grows with frequency, and adds no gain") {
  // Ten milliseconds at five hertz is a twentieth of a turn.
  RC_CHECK_NEAR(phase_lag_degrees(0.010, 5.0), 18.0, 1e-12);
  RC_CHECK_NEAR(phase_lag_degrees(0.010, 50.0), 180.0, 1e-12);   // at fifty hertz it is exactly backwards
  RC_CHECK_NEAR(phase_lag_degrees(0.0, 123.0), 0.0, 1e-12);
  RC_CHECK_NEAR(phase_lag_degrees(0.020, 5.0), 2.0 * phase_lag_degrees(0.010, 5.0), 1e-12);
}

RC_TEST("the crossover frequency is where the loop gain has size one") {
  double worst = 0.0;
  int tried = 0;
  for (const double kp : {1.0, 30.0, 1000.0, 50000.0}) {
    for (const double ratio : {0.2, 1.0, 3.0}) {
      const double kd = 2.0 * ratio * std::sqrt(kp * kInertia);
      const double mine = crossover_frequency(kp, kd, kInertia);
      const double searched = crossover_by_search(kp, kd, kInertia);
      worst = std::fmax(worst, std::fabs(mine - searched) / searched);
      RC_CHECK_NEAR(std::abs(loop_gain(kp, kd, kInertia, 0.0, mine)), 1.0, 1e-9);
      ++tried;
    }
  }

  std::cout << "  " << tried << " gains and dampings: the worst relative difference from a search for\n"
            << "  the frequency where the size is one was " << std::scientific << std::setprecision(1) << worst
            << ".\n\n";

  RC_CHECK_EQ(tried, 12);
  RC_CHECK(worst < 1e-9);
}

RC_TEST("the phase margin is what is left of 180 degrees, and agrees with the complex number") {
  double worst = 0.0;
  for (const double kp : {10.0, 300.0, 5000.0}) {
    for (const double delay : {0.0, 0.002, 0.010, 0.030}) {
      const double kd = 2.0 * std::sqrt(kp * kInertia);
      const double w = crossover_frequency(kp, kd, kInertia);
      const std::complex<double> l = loop_gain(kp, kd, kInertia, delay, w);

      // The argument of the loop gain at crossover, and how far it is from minus 180.
      double margin = std::arg(l) * 180.0 / kPi + 180.0;
      while (margin > 180.0) margin -= 360.0;
      while (margin < -180.0) margin += 360.0;

      // An angle is only known modulo a turn: a margin of minus 293 degrees and one of
      // plus 67 are the same direction, so the difference is taken round the circle.
      double difference = margin - phase_margin_degrees(kp, kd, kInertia, delay);
      while (difference > 180.0) difference -= 360.0;
      while (difference < -180.0) difference += 360.0;
      worst = std::fmax(worst, std::fabs(difference));
    }
  }
  RC_CHECK(worst < 1e-9);

  // And it only shrinks as the delay grows, because the delay only adds lag.
  const double kp = 1000.0;
  const double kd = 2.0 * std::sqrt(kp * kInertia);
  double previous = phase_margin_degrees(kp, kd, kInertia, 0.0);
  RC_CHECK(previous > 40.0);
  for (const double delay : {0.002, 0.005, 0.010, 0.020, 0.040}) {
    const double now = phase_margin_degrees(kp, kd, kInertia, delay);
    RC_CHECK(now < previous);
    previous = now;
  }
  RC_CHECK(previous < 0.0);   // forty milliseconds is too much for this gain
}

RC_TEST("the margin predicts the largest stable gain, and the gain falls as the square of the delay") {
  struct Row {
    double delay;
    double simulated;
    double predicted;
  };
  Row rows[5];
  const double delays[] = {0.0, 0.005, 0.010, 0.020, 0.040};

  for (int i = 0; i < 5; ++i) {
    rows[i].delay = delays[i];
    rows[i].simulated = simulated_limit(delays[i], 1.0);

    // Holding a command between samples costs half a sample, and the prediction has
    // to be told about it, because the simulation has it and the model does not.
    rows[i].predicted = largest_stable_gain(kInertia, delays[i] + kStep / 2.0, 1.0);
  }

  std::cout << "  the largest proportional gain at which a step settles, in a 1 kHz loop on a 0.5 kg m^2\n"
            << "  inertia with the derivative gain set for critical damping, by simulation and by the margin:\n\n"
            << "    delay in the loop   simulated limit   predicted from the margin   difference\n";
  for (int i = 0; i < 5; ++i) {
    std::cout << "    " << std::fixed << std::setprecision(1) << std::setw(9) << rows[i].delay * 1000.0 << " ms"
              << std::setprecision(0) << std::setw(17) << rows[i].simulated << std::setw(22) << rows[i].predicted
              << std::setprecision(1) << std::setw(18) << 100.0 * (rows[i].predicted / rows[i].simulated - 1.0)
              << " %\n";
  }
  std::cout << "\n  Each doubling of the delay cuts the limit about fourfold: " << std::setprecision(1)
            << rows[1].simulated / rows[2].simulated << ", " << rows[2].simulated / rows[3].simulated << ", "
            << rows[3].simulated / rows[4].simulated << ".\n\n";

  // A delay of five milliseconds or more is what limits the gain, and the margin
  // gets the limit to within a few percent.
  for (int i = 1; i < 5; ++i) {
    RC_CHECK(rows[i].simulated > 0.0);
    RC_CHECK(std::fabs(rows[i].predicted / rows[i].simulated - 1.0) < 0.08);
  }
  for (int i = 1; i < 4; ++i) {
    const double ratio = rows[i].simulated / rows[i + 1].simulated;
    RC_CHECK(ratio > 3.0 && ratio < 4.5);
  }

  // With no delay at all the model says there is no limit, and the loop still has
  // one: the sampling is a delay of its own.
  RC_CHECK(std::isinf(largest_stable_gain(kInertia, 0.0, 1.0)));
  RC_CHECK(rows[0].simulated > 1e4);
  RC_CHECK(rows[0].simulated < 1e7);
  RC_CHECK(rows[0].simulated > 20.0 * rows[1].simulated);
}

RC_TEST("what a delay does to a gain that was fine without it") {
  const double kp = 3000.0;
  const double kd = 2.0 * std::sqrt(kp * kInertia);

  std::cout << "  a proportional gain of " << std::fixed << std::setprecision(0) << kp << " with critical damping,\n"
            << "  as the delay in the loop grows:\n\n"
            << "    delay    phase margin    the step response\n";

  bool settled_without = false;
  bool settled_with_forty = true;
  for (const double delay : {0.0, 0.005, 0.010, 0.020, 0.040}) {
    const double margin = phase_margin_degrees(kp, kd, kInertia, delay + kStep / 2.0);
    const bool ok = settles(kp, kd, delay);
    std::cout << "    " << std::setprecision(1) << std::setw(4) << delay * 1000.0 << " ms" << std::setw(11) << margin
              << " deg    " << (ok ? "settles" : "grows without bound") << "\n";
    if (delay == 0.0) settled_without = ok;
    if (delay == 0.040) settled_with_forty = ok;

    // The prediction and the simulation agree on which side of zero it is.
    RC_CHECK_EQ(margin > 0.0, ok);
  }
  std::cout << "\n";

  RC_CHECK(settled_without);
  RC_CHECK(!settled_with_forty);
}
