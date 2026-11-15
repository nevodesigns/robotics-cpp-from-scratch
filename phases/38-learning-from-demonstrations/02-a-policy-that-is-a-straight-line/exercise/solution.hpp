#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cmath>
#include <rc/learn/dataset.hpp>
#include <vector>

using rc::learn::LaneTask;
using rc::learn::Sample;
using rc::learn::demonstrations;
using rc::learn::split_by_episode;

struct LinearPolicy {
  double w[2] = {0.0, 0.0};   // per metre of offset, per radian of heading error
  double b = 0.0;             // rad/s
};

inline double linear_act(const LinearPolicy& p, const double s[2]) { return p.w[0] * s[0] + p.w[1] * s[1] + p.b; }

// Solves A x = r for three unknowns by elimination with partial pivoting. A and r are overwritten.
inline bool solve3(double A[3][3], double r[3], double x[3]) {
  for (int c = 0; c < 3; ++c) {
    int p = c;
    for (int i = c + 1; i < 3; ++i)
      if (std::fabs(A[i][c]) > std::fabs(A[p][c])) p = i;
    if (std::fabs(A[p][c]) < 1e-12) return false;
    for (int j = 0; j < 3; ++j) {
      const double t = A[c][j];
      A[c][j] = A[p][j];
      A[p][j] = t;
    }
    const double tr = r[c];
    r[c] = r[p];
    r[p] = tr;
    for (int i = c + 1; i < 3; ++i) {
      const double f = A[i][c] / A[c][c];
      for (int j = c; j < 3; ++j) A[i][j] -= f * A[c][j];
      r[i] -= f * r[c];
    }
  }
  for (int i = 2; i >= 0; --i) {
    double s = r[i];
    for (int j = i + 1; j < 3; ++j) s -= A[i][j] * x[j];
    x[i] = s / A[i][i];
  }
  return true;
}

// Least squares with a ridge on the two weights (not on the constant). An empty or singular set gives the zero policy.
inline LinearPolicy fit_linear(const std::vector<Sample>& d, double ridge) {
  LinearPolicy p;
  (void)d;  // TODO: with the features (offset, heading, 1), add up the normal equations A = sum f f^T and r = sum f a over d; add ridge to the first two diagonal terms (not the constant's); solve with solve3 and return the three numbers as w[0], w[1] and b (an empty or singular set gives the zero policy)
  (void)ridge;
  return p;
}

inline double linear_mse(const LinearPolicy& p, const std::vector<Sample>& d) {
  double s = 0.0;
  for (const Sample& x : d) {
    const double e = linear_act(p, x.s) - x.a;
    s += e * e;
  }
  return d.empty() ? 0.0 : s / static_cast<double>(d.size());
}

// The share of the actions in d that are at the car's largest turn.
inline double saturated_share(const LaneTask& t, const std::vector<Sample>& d) {
  int n = 0;
  for (const Sample& x : d)
    if (std::fabs(x.a) >= t.max_turn - 1e-12) ++n;
  return d.empty() ? 0.0 : static_cast<double>(n) / static_cast<double>(d.size());
}


#endif
