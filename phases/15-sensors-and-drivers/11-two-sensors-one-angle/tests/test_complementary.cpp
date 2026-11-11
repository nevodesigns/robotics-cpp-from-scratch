#include <rc/test/rc_test.hpp>

#include <cmath>
#include <cstdint>
#include <iomanip>
#include <iostream>

#include "solution.hpp"

namespace {

struct Rng {
  std::uint32_t state;
  std::uint32_t next() {
    state ^= state << 13;
    state ^= state >> 17;
    state ^= state << 5;
    return state;
  }
  double uniform() { return static_cast<double>(next() >> 8) / 16777216.0; }
  double gaussian() {
    double sum = 0.0;
    for (int i = 0; i < 12; ++i) sum += uniform();
    return sum - 6.0;
  }
};

const double kDt = 0.01;            // a 100 Hz sensor
const double kBias = 0.02;          // the gyro reads 0.02 rad/s when it is still
const double kGyroNoise = 0.01;     // rad/s
const double kAccelNoise = 0.05;    // rad
const double kPi = 3.14159265358979323846;

// A joint swinging 0.5 rad either side every 5 seconds.
double truth_angle(double t) { return 0.5 * std::sin(2.0 * kPi * 0.2 * t); }
double truth_rate(double t) { return 0.5 * 2.0 * kPi * 0.2 * std::cos(2.0 * kPi * 0.2 * t); }

struct Errors {
  double gyro_alone = 0.0;
  double accel_alone = 0.0;
  double fused = 0.0;
};

// rms angle error over 60 s, after the first 10, of the gyro integrated alone, the accelerometer alone and a filter.
Errors run(double tau, bool moving, double bias) {
  Rng rng{3};
  ComplementaryFilter filter(tau);
  const double start = moving ? 0.0 : 0.3;
  filter.reset(start);
  double gyro_angle = start;
  double sum_gyro = 0.0;
  double sum_accel = 0.0;
  double sum_fused = 0.0;
  int counted = 0;
  const int steps = 6000;
  for (int i = 1; i <= steps; ++i) {
    const double t = i * kDt;
    const double angle = moving ? truth_angle(t) : 0.3;
    const double rate = moving ? truth_rate(t) : 0.0;
    const double gyro = rate + bias + kGyroNoise * rng.gaussian();
    const double accel = angle + kAccelNoise * rng.gaussian();
    gyro_angle += gyro * kDt;
    filter.update(gyro, accel, kDt);
    if (t < 10.0) continue;
    sum_gyro += (gyro_angle - angle) * (gyro_angle - angle);
    sum_accel += (accel - angle) * (accel - angle);
    sum_fused += (filter.angle() - angle) * (filter.angle() - angle);
    ++counted;
  }
  Errors e;
  e.gyro_alone = std::sqrt(sum_gyro / counted);
  e.accel_alone = std::sqrt(sum_accel / counted);
  e.fused = std::sqrt(sum_fused / counted);
  return e;
}

}  // namespace

RC_TEST("the share kept is tau over tau plus dt, and the time constant comes from the crossover") {
  RC_CHECK_NEAR(complementary_alpha(0.5, 0.01), 0.5 / 0.51, 1e-12);
  RC_CHECK_NEAR(complementary_alpha(0.0, 0.01), 0.0, 1e-12);
  RC_CHECK_NEAR(complementary_alpha(-1.0, 0.01), 0.0, 1e-12);
  RC_CHECK_NEAR(complementary_alpha(0.5, 0.0), 0.0, 1e-12);
  RC_CHECK_NEAR(tau_for_crossover(1.0), 1.0 / (2.0 * kPi), 1e-12);
  RC_CHECK_NEAR(tau_for_crossover(0.0), 0.0, 1e-12);
}

RC_TEST("a gyro alone drifts by its bias times the time, and an accelerometer alone is as noisy as it is") {
  const Errors e = run(1.0, false, kBias);
  // The gyro's error grows to 0.02 rad/s times the seconds elapsed: between 10 and 60 s that is 0.2 to 1.2 rad.
  const double predicted = kBias * std::sqrt((10.0 * 10.0 + 10.0 * 60.0 + 60.0 * 60.0) / 3.0);   // rms of a ramp
  std::cout << "  still joint, 60 s: gyro alone " << std::fixed << std::setprecision(3) << e.gyro_alone
            << " rad rms (a ramp of 0.02 rad/s predicts " << predicted << "), accelerometer alone " << e.accel_alone
            << " rad rms, filter " << e.fused << " rad rms\n\n";
  RC_CHECK(e.fused > 0.0 && e.fused < 0.5 * e.accel_alone);
  RC_CHECK(e.gyro_alone > 0.0);
  RC_CHECK_NEAR(e.gyro_alone, predicted, 0.05 * predicted);
  RC_CHECK_NEAR(e.accel_alone, kAccelNoise, 0.15 * kAccelNoise);
}

RC_TEST("a filter that believes both is better than either alone") {
  const Errors e = run(1.0, true, kBias);
  std::cout << "  swinging joint, tau 1 s: gyro " << std::fixed << std::setprecision(3) << e.gyro_alone
            << " rad rms, accelerometer " << e.accel_alone << ", filter " << e.fused << "\n\n";
  RC_CHECK(e.fused > 0.0);
  RC_CHECK(e.fused < 0.5 * e.accel_alone);
  RC_CHECK(e.fused < 0.1 * e.gyro_alone);
}

RC_TEST("a gyro bias leaves a steady error of the bias times tau, so a longer tau trusts the gyro more") {
  std::cout << "  still joint, bias 0.02 rad/s, no noise:  tau (s)   error (rad)   bias x tau\n";
  bool matches = true;
  for (double tau : {0.25, 0.5, 1.0, 2.0, 4.0}) {
    ComplementaryFilter filter(tau);
    filter.reset(0.3);
    double error = 0.0;
    for (int i = 0; i < 6000; ++i) {
      filter.update(kBias, 0.3, kDt);
      error = filter.angle() - 0.3;
    }
    const double predicted = kBias * tau;
    std::cout << "                                       " << std::setw(6) << std::fixed << std::setprecision(2) << tau
              << "   " << std::setw(11) << std::setprecision(4) << error << "   " << std::setw(10) << predicted << "\n";
    if (std::fabs(error - predicted) > 0.05 * predicted) matches = false;
  }
  std::cout << "\n";
  RC_CHECK(matches);
}

RC_TEST("one time constant cannot make both errors small, and the total is least between the extremes") {
  std::cout << "  tau (s)   crossover (Hz)   still, rms (rad)   swinging, rms (rad)\n";
  double best = 1e9;
  double best_tau = 0.0;
  double first = 0.0;
  double last = 0.0;
  const double taus[] = {0.05, 0.2, 0.5, 1.0, 2.0, 5.0, 20.0};
  for (double tau : taus) {
    const double still = run(tau, false, kBias).fused;
    const double swing = run(tau, true, kBias).fused;
    std::cout << "  " << std::setw(7) << std::fixed << std::setprecision(2) << tau << "   " << std::setw(14)
              << std::setprecision(3) << 1.0 / (2.0 * kPi * tau) << "   " << std::setw(16) << std::setprecision(4) << still
              << "   " << std::setw(19) << swing << "\n";
    if (swing < best) {
      best = swing;
      best_tau = tau;
    }
    if (tau == taus[0]) first = swing;
    last = swing;
  }
  std::cout << "  least error while swinging at a tau of " << std::setprecision(2) << best_tau << " s\n\n";
  RC_CHECK(best > 0.0);
  RC_CHECK(best < first);
  RC_CHECK(best < last);
}
