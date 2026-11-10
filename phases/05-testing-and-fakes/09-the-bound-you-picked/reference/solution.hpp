#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cmath>
#include <cstddef>
#include <functional>
#include <vector>

#include <rc/core/random.hpp>

using rc::core::Rng;

// The average of n readings of `truth`, each with noise of spread 1, plus `bias`, which is a bug when it is not zero.
//
// This is a stand-in for any quantity a test computes from noisy readings: its own value changes from seed to seed
// with a spread of about 1 / sqrt(n), and that spread, not the noise on one reading, is what a bound has to fit.
inline double noisy_average(unsigned seed, int n, double truth, double bias) {
  Rng rng(seed);
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

#endif  // LESSON_SOLUTION_HPP
