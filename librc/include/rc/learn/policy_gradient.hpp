// rc/learn/policy_gradient.hpp
//
// A policy gradient, from lesson 39-07.
//
// The task: a state x from a start within plus or minus 1, moved by x next = x + u for ten steps, with a reward of minus (x next squared plus c times u squared) at each step. The policy's action is u = -k x plus normal noise of spread sigma. pg_run makes one run and returns its return G and its score, the sum over the steps of the derivative of the log chance of each action with respect to k, which for this policy is ((u - mean) / sigma squared) times (-x). pg_estimate makes a batch of runs and estimates the gradient of the expected return with respect to k as the mean of score times (G minus b), with b zero or the batch's mean return (the baseline). reinforce starts from k = 0 and steps k by a learning rate times each estimate. mean_return is the noise-free return of a gain averaged over starts from a seed, and best_gain finds the best gain by scanning.
//
// The best gain by scanning was 0.916 (mean return -0.0302; with k = 0, -3.2990). From k = 0, 300 steps of batches of 10 runs, with noise 0.3 and a learning rate of 0.002, reached k = 0.898 without a baseline (return -0.0303) and 0.837 with one (return -0.0328). Over the first 50 steps, the spread of the gradient estimates was 29.2 without the baseline and 13.7 with it.

#ifndef RC_RC_LEARN_POLICY_GRADIENT_HPP
#define RC_RC_LEARN_POLICY_GRADIENT_HPP

#include <cmath>
#include <rc/learn/dataset.hpp>
#include <vector>

namespace rc {
namespace learn {

struct PgTask {
  double c = 0.1;    // the cost of effort
  int steps = 10;
};

// One run with gain k and noise sigma: returns G; *score gets the sum over steps of d log pi(u | x) / dk.
inline double pg_run(const PgTask& t, double k, double sigma, Rng& r, double* score) {
  double x = 2.0 * uniform(r) - 1.0, G = 0.0, sc = 0.0;
  for (int i = 0; i < t.steps; ++i) {
    const double mean = -k * x;
    const double u = mean + sigma * normal(r);
    sc += (u - mean) / (sigma * sigma) * (-x);
    x = x + u;
    G += -(x * x + t.c * u * u);
  }
  if (score) *score = sc;
  return G;
}

// The gradient of the expected return with respect to k, from `batch` runs: the mean of score (G - b), b zero or the batch's mean return.
inline double pg_estimate(const PgTask& t, double k, double sigma, int batch, bool baseline, Rng& r) {
  std::vector<double> G(static_cast<std::size_t>(batch)), S(static_cast<std::size_t>(batch));
  for (int i = 0; i < batch; ++i) G[static_cast<std::size_t>(i)] = pg_run(t, k, sigma, r, &S[static_cast<std::size_t>(i)]);
  double grad = 0.0;
  double b = 0.0;
  if (baseline) {
    for (double g : G) b += g;
    b /= batch;
  }
  for (int i = 0; i < batch; ++i) grad += S[static_cast<std::size_t>(i)] * (G[static_cast<std::size_t>(i)] - b);
  grad /= batch;
  return grad;
}

// From k = 0, `iters` steps of k += lr times an estimate. *estimates, if not null, gets every estimate.
inline double reinforce(const PgTask& t, double sigma, double lr, int batch, int iters, bool baseline, unsigned long long seed,
                        std::vector<double>* estimates) {
  Rng r = make_rng(seed);
  double k = 0.0;
  if (estimates) estimates->clear();
  for (int i = 0; i < iters; ++i) {
    const double g = pg_estimate(t, k, sigma, batch, baseline, r);
    if (estimates) estimates->push_back(g);
    k += lr * g;
  }
  return k;
}

// The noise-free return of gain k, averaged over n starts from seed.
inline double mean_return(const PgTask& t, double k, int n, unsigned long long seed) {
  Rng r = make_rng(seed);
  double s = 0.0;
  for (int i = 0; i < n; ++i) {
    double x = 2.0 * uniform(r) - 1.0, G = 0.0;
    for (int j = 0; j < t.steps; ++j) {
      const double u = -k * x;
      x = x + u;
      G += -(x * x + t.c * u * u);
    }
    s += G;
  }
  return n > 0 ? s / n : 0.0;
}

// The best gain on a grid of `points` values from 0 to hi.
inline double best_gain(const PgTask& t, double hi, int points, int n, unsigned long long seed) {
  double best = 0.0, bv = mean_return(t, 0.0, n, seed);
  for (int i = 1; i <= points; ++i) {
    const double k = hi * i / points;
    const double v = mean_return(t, k, n, seed);
    if (v > bv) {
      bv = v;
      best = k;
    }
  }
  return best;
}


}  // namespace learn
}  // namespace rc

#endif
