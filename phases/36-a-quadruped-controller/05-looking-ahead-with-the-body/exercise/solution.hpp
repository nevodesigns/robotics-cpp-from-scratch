#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cmath>

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
  (void)x0;  // TODO: predictions s_k = A^k s0 + sum over j < k of A^(k-1-j) B u_j (rows for the position and velocity at each of the n steps, a matrix Phi in the commands and a vector F from the free motion); cost = sum of qx x_k^2 + qv v_k^2 + r u_j^2; H = 2 (Phi^T Q Phi + r I) and g = 2 Phi^T Q F; start from u = 0 and repeat: u = clip(u - (1 / trace H) (H u + g), -bound[k], bound[k]) for each command k; return the first command (and fill u_all)
  (void)v0;
  (void)h;
  (void)n;
  (void)bound;  // an array: one bound for each step ahead
  (void)qx;
  (void)qv;
  (void)r;
  (void)iters;
  (void)Ap;
  if (u_all)
    for (int a = 0; a < n; ++a) u_all[a] = 0.0;
  return 0.0;
}


#endif
