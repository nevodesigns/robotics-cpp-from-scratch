// rc/sim/compare.hpp
//
// Comparing a simulated trace with a measured one, from lesson 18-08, graduated.
//
// The error (rms and largest), the bias, and the shift in samples at which the traces fit best, found from the variance of the difference so that a constant offset does not move it; the noise floor of the measurement itself, from two real runs of the same thing; and limits a simulation must
// stay within, with the name of each one exceeded.
//
// A trace 0.05 too high gave rmse 0.0500, bias 0.0500 and lag 0. A motor's speed read through a sensor 5 samples late (open loop) had an error of 0.4042 lined up and, at the best shift, 5, an error of 0.0000; inside a closed loop the same delay gave 0.1553 lined up, a best shift of 4 and 0.1396 at that shift.
// With sensor noise 0.01 the noise floor was 0.0102 and a perfect noiseless model scored 0.0105. The error against a real motor of time constant 0.12 s was 0.0810, 0.0501, 0.0272, 0.0098, 0.0374, 0.0930 and 0.1822 for models of 0.05, 0.08, 0.10, 0.12, 0.15, 0.20 and 0.30.

#ifndef RC_SIM_COMPARE_HPP
#define RC_SIM_COMPARE_HPP

#include <cmath>
#include <cstddef>
#include <string>
#include <vector>

namespace rc {
namespace sim {

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
  Metrics m;
  const std::size_t n = shorter(sim, real);
  if (n == 0) return m;
  double sq = 0.0;
  double sum = 0.0;
  for (std::size_t i = 0; i < n; ++i) {
    const double d = sim[i] - real[i];
    sq += d * d;
    sum += d;
    if (std::fabs(d) > m.max_abs) m.max_abs = std::fabs(d);
  }
  m.rmse = std::sqrt(sq / static_cast<double>(n));
  m.bias = sum / static_cast<double>(n);

  double best = 1e300;
  int best_lag = 0;
  for (int step = 0; step <= 2 * max_lag; ++step) {
    // 0, 1, -1, 2, -2, ... so that the smallest shift is tried first and keeps a tie
    const int signed_lag = (step % 2 == 1) ? (step + 1) / 2 : -(step / 2);
    double s = 0.0;
    double ss = 0.0;
    int count = 0;
    for (std::size_t i = 0; i < n; ++i) {
      const long j = static_cast<long>(i) + signed_lag;
      if (j < 0 || j >= static_cast<long>(real.size())) continue;
      const double d = sim[i] - real[static_cast<std::size_t>(j)];
      s += d;
      ss += d * d;
      ++count;
    }
    if (count == 0) continue;
    const double mean = s / count;
    double variance = ss / count - mean * mean;
    if (variance < 0.0) variance = 0.0;
    if (variance < best - 1e-18) {
      best = variance;
      best_lag = signed_lag;
    }
  }
  m.lag = best_lag;
  m.rmse_at_lag = std::sqrt(best);
  return m;
}

// The noise in the measurement itself: two real runs of the same thing differ only by noise, and the difference has twice the variance of one.
// So the noise floor, the rms noise on one run, is the rms difference of the two divided by the square root of two. Zero if there is nothing to compare.
inline double noise_floor(const std::vector<double>& run_a, const std::vector<double>& run_b) {
  const std::size_t n = shorter(run_a, run_b);
  if (n == 0) return 0.0;
  double sq = 0.0;
  for (std::size_t i = 0; i < n; ++i) sq += (run_a[i] - run_b[i]) * (run_a[i] - run_b[i]);
  return std::sqrt(sq / static_cast<double>(n)) / std::sqrt(2.0);
}

// What a simulation must stay within to be trusted.
struct Limits {
  double rmse = 0.0;
  double max_abs = 0.0;
  int lag = 0;                  // the largest shift, either way, in samples
};

// The names of the limits that were exceeded: "rmse", "max", "lag". Empty means within them all.
inline std::vector<std::string> exceeded(const Metrics& m, const Limits& limits) {
  std::vector<std::string> out;
  if (m.rmse > limits.rmse) out.push_back("rmse");
  if (m.max_abs > limits.max_abs) out.push_back("max");
  if (std::abs(m.lag) > limits.lag) out.push_back("lag");
  return out;
}

}  // namespace sim
}  // namespace rc

#endif  // RC_SIM_COMPARE_HPP
