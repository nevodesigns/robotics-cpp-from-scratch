#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cmath>
#include <rc/learn/dataset.hpp>
#include <vector>

using rc::learn::Rng;
using rc::learn::make_rng;
using rc::learn::uniform;

struct Mlp {
  int hidden = 0;
  std::vector<double> w1;   // 2 per hidden unit: w1[2 i] for input 0, w1[2 i + 1] for input 1
  std::vector<double> b1;   // one per hidden unit
  std::vector<double> w2;   // one per hidden unit
  double b2 = 0.0;
};

inline Mlp make_mlp(int hidden, double scale, unsigned long long seed) {
  Mlp m;
  m.hidden = hidden;
  m.w1.assign(static_cast<std::size_t>(2 * hidden), 0.0);
  m.b1.assign(static_cast<std::size_t>(hidden), 0.0);
  m.w2.assign(static_cast<std::size_t>(hidden), 0.0);
  Rng r = make_rng(seed);
  for (double& w : m.w1) w = scale * (2.0 * uniform(r) - 1.0);
  for (double& b : m.b1) b = 0.1 * (2.0 * uniform(r) - 1.0);
  for (double& w : m.w2) w = (2.0 * uniform(r) - 1.0) / std::sqrt(static_cast<double>(hidden));
  return m;
}

inline int param_count(const Mlp& m) { return 4 * m.hidden + 1; }

// The output for input x; if z is not null it receives the hidden activations (it must have `hidden` entries).
inline double mlp_forward(const Mlp& m, const double x[2], std::vector<double>* z) {
  double y = m.b2;
  for (int i = 0; i < m.hidden; ++i) {
    const std::size_t k = static_cast<std::size_t>(i);
    const double t = std::tanh(m.w1[2 * k] * x[0] + m.w1[2 * k + 1] * x[1] + m.b1[k]);
    if (z) (*z)[k] = t;
    y += m.w2[k] * t;
  }
  return y;
}

struct MlpGrad {
  std::vector<double> w1, b1, w2;
  double b2 = 0.0;
};

// The derivative of (y - target)^2 / 2 with respect to every weight, by backpropagation. Returns the error y - target.
inline double mlp_gradient(const Mlp& m, const double x[2], double target, MlpGrad& g) {
  const std::size_t h = static_cast<std::size_t>(m.hidden);
  g.w1.assign(2 * h, 0.0);
  g.b1.assign(h, 0.0);
  g.w2.assign(h, 0.0);
  g.b2 = 0.0;
  std::vector<double> z(h, 0.0);
  const double e = mlp_forward(m, x, &z) - target;
  g.b2 = e;
  for (std::size_t i = 0; i < h; ++i) {
    g.w2[i] = e * z[i];
    const double dpre = e * m.w2[i] * (1.0 - z[i] * z[i]);
    g.w1[2 * i] = dpre * x[0];
    g.w1[2 * i + 1] = dpre * x[1];
    g.b1[i] = dpre;
  }
  return e;
}

// The largest relative difference between mlp_gradient and a central finite difference with step h, over every weight.
inline double numeric_check(Mlp m, const double x[2], double target, double h) {
  MlpGrad g;
  mlp_gradient(m, x, target, g);
  double worst = 0.0;
  // every weight in turn: nudge it up and down and compare the change of the loss with the analytic derivative
  std::vector<double*> params;
  std::vector<double> analytic;
  for (std::size_t k = 0; k < m.w1.size(); ++k) { params.push_back(&m.w1[k]); analytic.push_back(g.w1[k]); }
  for (std::size_t k = 0; k < m.b1.size(); ++k) { params.push_back(&m.b1[k]); analytic.push_back(g.b1[k]); }
  for (std::size_t k = 0; k < m.w2.size(); ++k) { params.push_back(&m.w2[k]); analytic.push_back(g.w2[k]); }
  params.push_back(&m.b2);
  analytic.push_back(g.b2);
  for (std::size_t k = 0; k < params.size(); ++k) {
    const double keep = *params[k];
    *params[k] = keep + h;
    const double up = mlp_forward(m, x, nullptr) - target;
    *params[k] = keep - h;
    const double down = mlp_forward(m, x, nullptr) - target;
    *params[k] = keep;
    const double numeric = (0.5 * up * up - 0.5 * down * down) / (2.0 * h);
    const double scale = std::fabs(numeric) + std::fabs(analytic[k]) + 1e-12;
    const double rel = std::fabs(numeric - analytic[k]) / scale;
    if (rel > worst) worst = rel;
  }
  return worst;
}


#endif
