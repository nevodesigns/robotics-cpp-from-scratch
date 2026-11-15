#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cmath>
#include <rc/learn/dataset.hpp>
#include <vector>

using rc::learn::Rng;
using rc::learn::make_rng;
using rc::learn::uniform;
using rc::learn::normal;

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


#endif
