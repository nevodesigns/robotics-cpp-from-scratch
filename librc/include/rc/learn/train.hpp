// rc/learn/train.hpp
//
// Training a network, from lesson 38-04.
//
// sgd_step moves every weight of the network by the step size times its gradient on one sample. train_epoch visits every sample of a set once, in an order shuffled from the random stream (a Fisher-Yates shuffle), with the inputs scaled by the given Normaliser. mlp_mse is the network's mean squared error on a set. train_with_stopping trains for up to a number of epochs and, after each, measures the error on a validation set: it keeps the weights of the best epoch and stops when the validation error has not improved for `patience` epochs; TrainResult says which epoch was best, its validation error, and the epoch at which training stopped.
//
// With 16 hidden units for 20 epochs, a step of 0.001 left the test error at 0.00243, a step of 0.02 brought it to 0.00130, and a step of 1.0 made the weights blow up. On a set of only 30 noisy samples, the training error fell from 0.04970 to 0.00012 over 3000 epochs while the validation error was lowest, 0.0280, at epoch 346, and ended at 0.0448; stopping with a patience of 200 stopped at epoch 546 and kept the weights of epoch 346. On demonstrations that reached the car's limit, the network's test error was 0.00034 against 0.01240 for the straight line.

#ifndef RC_RC_LEARN_TRAIN_HPP
#define RC_RC_LEARN_TRAIN_HPP

#include <cmath>
#include <rc/learn/dataset.hpp>
#include <rc/learn/linear_policy.hpp>
#include <rc/learn/mlp.hpp>
#include <vector>

namespace rc {
namespace learn {

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
  for (std::size_t i = order.size(); i > 1; --i) {
    const std::size_t j = static_cast<std::size_t>(uniform(r) * static_cast<double>(i));
    const std::size_t t = order[i - 1];
    order[i - 1] = order[j];
    order[j] = t;
  }
  for (std::size_t i : order) {
    double x[2];
    normalise(n, d[i].s, x);
    sgd_step(m, x, d[i].a, lr);
  }
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


}  // namespace learn
}  // namespace rc

#endif
