// rc/legged/body_mpc.hpp
//
// Looking ahead with the body, from lesson 36-05.
//
// lipm_discretize gives the exact model of the body over a time step dt: with w the square root of g over h, the position and velocity after dt are A times the present ones plus B times the ZMP, A = [[cosh w dt, sinh(w dt) / w], [w sinh w dt, cosh w dt]] and B = [1 - cosh w dt, -w sinh w dt]. mpc_zmp takes the position and velocity (relative to the middle of the support), a horizon of n steps (at most 20), the bound L on the ZMP and the weights on the position, the velocity and the ZMP, and finds the sequence of n ZMP commands within the bounds that makes the cost smallest: the predictions of position and velocity at each step ahead are matrices times the commands plus the free motion, the cost is quadratic in the commands, and a projected gradient method (steps of one over a bound on the Hessian's size, each followed by clipping to the bounds) finds the minimum. It returns the first command. pd_zmp is the fixed gain law kx x + kv v clipped to the bound.
//
// With a ZMP free to move 15 cm either side of the middle of the support and a body 0.27 m high, with the bound wide for 0.2 s and narrow (1 cm) for 0.2 s alternately (the feet landing), a push that gave the body a speed of 0.2 m/s was recovered by both laws; the largest push recovered by the fixed gain law (with the gains 3.0 and 0.9, clipped) was 0.70 m/s, by the predictive controller 0.71 m/s, and the limit with the bound always wide (the capture point of 35-07 at the bound) 0.90 m/s. For a push of 0.6 m/s the body was back at the middle after 1.00 s with the fixed gain law and 0.60 s with the predictive controller; for a small one of 0.3 m/s the fixed gain law took 0.00 s and the predictive controller 0.35. The first command of the predictive controller for a push of 0.5 m/s was 14.8 cm, and it needed 300 iterations to find.

#ifndef RC_RC_LEGGED_BODY_MPC_HPP
#define RC_RC_LEGGED_BODY_MPC_HPP

#include <cmath>

namespace rc {
namespace legged {

// The exact discrete model of x'' = (g / h) (x - p) over dt: s' = A s + B p, with s = (x, v).
inline void lipm_discretize(double h, double dt, double A[2][2], double B[2]) {
  const double w = std::sqrt(9.81 / h);
  const double c = std::cosh(w * dt), s = std::sinh(w * dt);
  A[0][0] = c;
  A[0][1] = s / w;
  A[1][0] = w * s;
  A[1][1] = c;
  B[0] = 1.0 - c;
  B[1] = -w * s;
}

// The fixed gain law, clipped to the bound.
inline double pd_zmp(double x, double v, double kx, double kv, double bound) {
  double p = kx * x + kv * v;
  if (p > bound) p = bound;
  if (p < -bound) p = -bound;
  return p;
}

// The first of the n ZMP commands (command k within plus or minus bound[k], which can change from step to step as the support does) that make the predicted cost smallest, found by a projected gradient method (n at most 20).
inline double mpc_zmp(double x0, double v0, double h, double dt, int n, const double* bound, double qx, double qv, double r, int iters, double* u_all) {
  double A[2][2], B[2];
  lipm_discretize(h, dt, A, B);
  // powers of A
  double Ap[21][2][2];
  Ap[0][0][0] = 1.0; Ap[0][0][1] = 0.0; Ap[0][1][0] = 0.0; Ap[0][1][1] = 1.0;
  for (int k = 1; k <= n; ++k)
    for (int i = 0; i < 2; ++i)
      for (int j = 0; j < 2; ++j) Ap[k][i][j] = A[i][0] * Ap[k - 1][0][j] + A[i][1] * Ap[k - 1][1][j];
  // predictions: s_k = A^k s0 + sum_{j < k} A^(k-1-j) B u_j, for k = 1..n; rows 2(k-1) (position) and 2(k-1)+1 (velocity)
  double Phi[40][20], F[40];
  for (int k = 1; k <= n; ++k) {
    F[2 * (k - 1)] = Ap[k][0][0] * x0 + Ap[k][0][1] * v0;
    F[2 * (k - 1) + 1] = Ap[k][1][0] * x0 + Ap[k][1][1] * v0;
    for (int j = 0; j < n; ++j) {
      if (j < k) {
        const double* a0 = Ap[k - 1 - j][0];
        const double* a1 = Ap[k - 1 - j][1];
        Phi[2 * (k - 1)][j] = a0[0] * B[0] + a0[1] * B[1];
        Phi[2 * (k - 1) + 1][j] = a1[0] * B[0] + a1[1] * B[1];
      } else {
        Phi[2 * (k - 1)][j] = 0.0;
        Phi[2 * (k - 1) + 1][j] = 0.0;
      }
    }
  }
  // cost = sum q (Phi u + F)^2 + r u^2; H = 2 (Phi^T Q Phi + r I), g = 2 Phi^T Q F
  double H[20][20], g[20];
  double trace = 0.0;
  for (int a = 0; a < n; ++a) {
    g[a] = 0.0;
    for (int row = 0; row < 2 * n; ++row) g[a] += 2.0 * ((row % 2 == 0) ? qx : qv) * Phi[row][a] * F[row];
    for (int b = 0; b < n; ++b) {
      double sum = 0.0;
      for (int row = 0; row < 2 * n; ++row) sum += ((row % 2 == 0) ? qx : qv) * Phi[row][a] * Phi[row][b];
      H[a][b] = 2.0 * sum + (a == b ? 2.0 * r : 0.0);
    }
    trace += H[a][a];
  }
  double u[20];
  for (int a = 0; a < n; ++a) u[a] = 0.0;
  const double step = 1.0 / (trace > 1e-12 ? trace : 1.0);
  for (int it = 0; it < iters; ++it) {
    double grad[20];
    for (int a = 0; a < n; ++a) {
      grad[a] = g[a];
      for (int b = 0; b < n; ++b) grad[a] += H[a][b] * u[b];
    }
    for (int a = 0; a < n; ++a) {
      u[a] -= step * grad[a];
      if (u[a] > bound[a]) u[a] = bound[a];
      if (u[a] < -bound[a]) u[a] = -bound[a];
    }
  }
  if (u_all)
    for (int a = 0; a < n; ++a) u_all[a] = u[a];
  return u[0];
}


}  // namespace legged
}  // namespace rc

#endif
