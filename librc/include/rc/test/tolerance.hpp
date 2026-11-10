// rc/test/tolerance.hpp
//
// A bound for a test with noise in it, from lesson 05-09, graduated.
//
// A quantity computed from noisy readings varies from seed to seed, and a bound has to fit that spread and not the noise on one reading. The average of 100 readings had a mean of 4.9988 and a
// spread of 0.0997 over 10000 seeds. A bound of one spread (0.1) passed the first run and failed 31.8 percent of 10000 (3176); three spreads failed 26 times and six spreads failed 0. A bound of six spreads still
// caught a bias: with 100 readings a bias of 0.3 was seen 0.1 percent of the time, 0.5 in 15.1, 0.8 in 97.0 and 1.0 always. samples_needed gives the readings for a bias to sit twice as far out as the bound: 1600 for a bias of 0.3
// at six spreads, which saw it in 2000 of 2000 runs with 0 false alarms in 10000.

#ifndef RC_TEST_TOLERANCE_HPP
#define RC_TEST_TOLERANCE_HPP

#include <cmath>
#include <cstddef>
#include <functional>
#include <vector>

#include <rc/core/random.hpp>

namespace rc {
namespace test {

// The average of n readings of `truth`, each with noise of spread 1, plus `bias`, which is a bug when it is not zero.
//
// This is a stand-in for any quantity a test computes from noisy readings: its own value changes from seed to seed
// with a spread of about 1 / sqrt(n), and that spread, not the noise on one reading, is what a bound has to fit.
inline double noisy_average(unsigned seed, int n, double truth, double bias) {
  rc::core::Rng rng(seed);
  double sum = 0.0;
  for (int i = 0; i < n; ++i) sum += truth + bias + rng.gaussian();
  return sum / n;
}

struct Spread {
  double mean = 0.0;
  double stddev = 0.0;
};

// The mean and the standard deviation (the spread) of some numbers. With fewer than two there is no spread to measure.
inline Spread mean_stddev(const std::vector<double>& values) {
  Spread s;
  if (values.empty()) return s;
  double sum = 0.0;
  for (double v : values) sum += v;
  s.mean = sum / static_cast<double>(values.size());
  if (values.size() < 2) return s;
  double squares = 0.0;
  for (double v : values) squares += (v - s.mean) * (v - s.mean);
  s.stddev = std::sqrt(squares / static_cast<double>(values.size() - 1));
  return s;
}

// A bound that is `k` spreads wide. A statistic that varies by chance is outside k spreads of its mean about once
// in 370 runs for k = 3 and about once in five hundred million for k = 6.
inline double bound_for(double spread, double k) { return k * spread; }

// How many readings a test needs to see a bias of `bias` in readings with noise of spread `noise`.
//
// The average of n readings has a spread of noise / sqrt(n). The bound is k of those. For the bias to sit well
// outside it, twice the bound, the number of readings is (2 k noise / bias) squared, rounded up. Zero if the bias is
// not above zero, since nothing can be seen.
inline int samples_needed(double noise, double bias, double k) {
  if (!(bias > 0.0)) return 0;
  const double n = (2.0 * k * noise / bias) * (2.0 * k * noise / bias);
  return static_cast<int>(std::ceil(n));
}

// Run a test on `count` different seeds, starting at `first`, and say how many times it failed.
inline int count_failures(const std::function<bool(unsigned)>& test, unsigned first, int count) {
  int failures = 0;
  for (int i = 0; i < count; ++i) {
    if (!test(first + static_cast<unsigned>(i))) ++failures;
  }
  return failures;
}

}  // namespace test
}  // namespace rc

#endif  // RC_TEST_TOLERANCE_HPP
