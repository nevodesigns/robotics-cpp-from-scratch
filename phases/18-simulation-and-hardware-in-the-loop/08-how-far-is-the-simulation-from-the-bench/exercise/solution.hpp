#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cmath>
#include <cstddef>
#include <string>
#include <vector>

// How far one trace is from another.
struct Metrics {
  double rmse = 0.0;      // root mean square of (sim - real), with the two lined up sample for sample
  double max_abs = 0.0;   // the largest single difference
  double bias = 0.0;      // the mean of (sim - real): does the simulation sit high or low
  int lag = 0;            // how many samples later the real trace is than the simulation, positive when the real one is late; the shift that fits best
  double rmse_at_lag = 0.0;   // the root mean square difference once the real trace is shifted back by `lag` and the constant bias is taken out
};

inline std::size_t shorter(const std::vector<double>& a, const std::vector<double>& b) { return a.size() < b.size() ? a.size() : b.size(); }

// Compare a simulated trace with a real one, sample for sample over as many points as both have. Then try every shift of the real trace from
// minus `max_lag` to plus `max_lag` samples and keep the one with the smallest rms difference (the smallest shift wins a tie).
// Real late by L means sim[i] is compared with real[i + L]. The shifts are judged by how much the difference varies (its variance), not by its size, so that a constant
// offset between the traces does not pull the shift away from where it really is.
inline Metrics compare(const std::vector<double>& sim, const std::vector<double>& real, int max_lag) {
  // TODO: Over n points, n the smaller of the two sizes (no points: the empty Metrics): rmse = the square root of the mean of (sim[i] - real[i]) squared; max_abs = the largest |difference|; bias = the mean of (sim[i] - real[i]).
  // Then the shift: for each lag from -max_lag to max_lag, trying 0 first, then 1, -1, 2, -2 and so on (so that the smallest shift wins a tie), take the differences d = sim[i] - real[i + lag] over the i for which i + lag is inside real,
  // and work out their VARIANCE: mean of d squared minus the square of the mean of d (never below zero). The lag with the smallest variance is `lag`, and the square root of that smallest variance is rmse_at_lag.
  // Judging by variance and not by size means a constant offset between the traces does not move the answer. Real late by L means sim[i] is compared with real[i + L].
  (void)sim;
  (void)real;
  (void)max_lag;
  return Metrics();
}

// The noise in the measurement itself: two real runs of the same thing differ only by noise, and the difference has twice the variance of one.
// So the noise floor, the rms noise on one run, is the rms difference of the two divided by the square root of two. Zero if there is nothing to compare.
inline double noise_floor(const std::vector<double>& run_a, const std::vector<double>& run_b) {
  // TODO: the rms of the differences between the two runs over the points both have, divided by the square root of 2. Zero if there are none
  (void)run_a;
  (void)run_b;
  return 0.0;
}

// What a simulation must stay within to be trusted.
struct Limits {
  double rmse = 0.0;
  double max_abs = 0.0;
  int lag = 0;                  // the largest shift, either way, in samples
};

// The names of the limits that were exceeded: "rmse", "max", "lag". Empty means within them all.
inline std::vector<std::string> exceeded(const Metrics& m, const Limits& limits) {
  // TODO: the names \"rmse\" (rmse over its limit), \"max\" (max_abs over its limit) and \"lag\" (the absolute lag over its limit), in that order, for each one exceeded
  (void)m;
  (void)limits;
  return std::vector<std::string>();
}

#endif  // LESSON_SOLUTION_HPP
