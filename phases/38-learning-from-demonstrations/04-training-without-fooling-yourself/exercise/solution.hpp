#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cmath>
#include <rc/learn/dataset.hpp>
#include <rc/learn/linear_policy.hpp>
#include <rc/learn/mlp.hpp>
#include <vector>

using rc::learn::Rng;
using rc::learn::make_rng;
using rc::learn::uniform;
using rc::learn::Sample;
using rc::learn::Normaliser;
using rc::learn::normalise;
using rc::learn::fit_normaliser;
using rc::learn::Mlp;
using rc::learn::MlpGrad;
using rc::learn::make_mlp;
using rc::learn::mlp_forward;
using rc::learn::mlp_gradient;
using rc::learn::LaneTask;
using rc::learn::demonstrations;
using rc::learn::split_by_episode;
using rc::learn::fit_linear;
using rc::learn::linear_mse;

// One step of gradient descent on one sample whose inputs are already scaled.
inline void sgd_step(Mlp& m, const double x[2], double target, double lr) {
  MlpGrad g;
  mlp_gradient(m, x, target, g);
  for (std::size_t k = 0; k < m.w1.size(); ++k) m.w1[k] -= lr * g.w1[k];
  for (std::size_t k = 0; k < m.b1.size(); ++k) m.b1[k] -= lr * g.b1[k];
  for (std::size_t k = 0; k < m.w2.size(); ++k) m.w2[k] -= lr * g.w2[k];
  m.b2 -= lr * g.b2;
}

// Every sample of d once, in an order shuffled from r, with the inputs scaled by n.
inline void train_epoch(Mlp& m, const Normaliser& n, const std::vector<Sample>& d, double lr, Rng& r) {
  std::vector<std::size_t> order(d.size());
  for (std::size_t i = 0; i < d.size(); ++i) order[i] = i;
  (void)m;  // TODO: shuffle `order` (for i from its size down to 2: j = the integer part of uniform(r) times i, swap order[i - 1] and order[j]); then for each index in that order, scale the sample's state with normalise and take one sgd_step toward its action
  (void)n;
  (void)lr;
  (void)r;
}

inline double mlp_mse(const Mlp& m, const Normaliser& n, const std::vector<Sample>& d) {
  double s = 0.0;
  for (const Sample& x : d) {
    double z[2];
    normalise(n, x.s, z);
    const double e = mlp_forward(m, z, nullptr) - x.a;
    s += e * e;
  }
  return d.empty() ? 0.0 : s / static_cast<double>(d.size());
}

struct TrainResult {
  int best_epoch = 0;        // the epoch whose weights were kept (0: the starting weights)
  double best_val = 0.0;
  int stopped_at = 0;        // the last epoch trained
};

// Trains for up to `epochs`, keeps the weights with the lowest validation error, stops after `patience` epochs with no improvement.
inline TrainResult train_with_stopping(Mlp& m, const Normaliser& n, const std::vector<Sample>& train, const std::vector<Sample>& val,
                                       int epochs, double lr, int patience, unsigned long long seed) {
  Rng r = make_rng(seed);
  TrainResult res;
  res.best_val = mlp_mse(m, n, val);
  Mlp best = m;
  for (int ep = 1; ep <= epochs; ++ep) {
    train_epoch(m, n, train, lr, r);
    res.stopped_at = ep;
    const double v = mlp_mse(m, n, val);
    if (v < res.best_val) {
      res.best_val = v;
      res.best_epoch = ep;
      best = m;
    } else if (ep - res.best_epoch >= patience) {
      break;
    }
  }
  m = best;
  return res;
}


#endif
