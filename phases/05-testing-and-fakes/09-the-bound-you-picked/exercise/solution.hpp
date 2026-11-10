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
  // TODO: no values gives a mean of 0 and a spread of 0. Otherwise the mean is the sum divided by the count.
  // With two or more values the spread is the square root of (the sum of (v - mean)^2) divided by (count - 1).
  // With one value the spread stays 0.
  (void)values;
  return Spread();
}

// A bound that is `k` spreads wide.
inline double bound_for(double spread, double k) {
  // TODO: k times the spread.
  (void)spread;
  (void)k;
  return 0.0;
}

// How many readings a test needs to see a bias of `bias` in readings with noise of spread `noise`.
inline int samples_needed(double noise, double bias, double k) {
  // TODO: zero if bias is not above zero. Otherwise (2 * k * noise / bias) squared, rounded up with
  // std::ceil and returned as an int. The average of n readings has spread noise / sqrt(n); the bound is k
  // of those; and the bias should be twice the bound, so it sits well outside it.
  (void)noise;
  (void)bias;
  (void)k;
  return -1;
}

// Run a test on `count` different seeds, starting at `first`, and say how many times it failed.
inline int count_failures(const std::function<bool(unsigned)>& test, unsigned first, int count) {
  // TODO: call test(first + i) for i from 0 to count - 1, and count the calls that returned false.
  (void)test;
  (void)first;
  (void)count;
  return -1;
}

#endif  // LESSON_SOLUTION_HPP
