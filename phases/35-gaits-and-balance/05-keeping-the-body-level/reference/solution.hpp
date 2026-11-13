#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cmath>

// The moments (about the roll and pitch axes) that pull the body back: a PD law on the angle and its rate.
inline double balance_moment(double angle, double rate, double kp, double kd) { return -kp * angle - kd * rate; }

// The moments that the vertical forces f (on the feet at (x, y), from above, about the centre of mass) give: Mx = sum f y, My = -sum f x.
inline void realized_moments(const double feet[4][2], const double f[4], double* mx, double* my) {
  double a = 0.0, b = 0.0;
  for (int i = 0; i < 4; ++i) {
    a += f[i] * feet[i][1];
    b -= f[i] * feet[i][0];
  }
  *mx = a;
  *my = b;
}

// The forces on the feet on the ground that add to the weight W and give the moments (mx, my), by the least norm solution; they may be negative. False if fewer than three feet are down or the feet are in a line.
inline bool foot_forces_raw(const double feet[4][2], const bool on_ground[4], double W, double mx, double my, double f[4]) {
  for (int i = 0; i < 4; ++i) f[i] = 0.0;
  int idx[4], n = 0;
  for (int i = 0; i < 4; ++i)
    if (on_ground[i]) idx[n++] = i;
  if (n < 3) return false;
  double a[3][4];
  for (int k = 0; k < n; ++k) {
    a[0][k] = 1.0;
    a[1][k] = feet[idx[k]][1];
    a[2][k] = -feet[idx[k]][0];
  }
  const double w[3] = {W, mx, my};
  double m[3][3];
  for (int r = 0; r < 3; ++r)
    for (int c = 0; c < 3; ++c) {
      double sum = 0.0;
      for (int k = 0; k < n; ++k) sum += a[r][k] * a[c][k];
      m[r][c] = sum;
    }
  const double d = m[0][0] * (m[1][1] * m[2][2] - m[1][2] * m[2][1]) - m[0][1] * (m[1][0] * m[2][2] - m[1][2] * m[2][0]) + m[0][2] * (m[1][0] * m[2][1] - m[1][1] * m[2][0]);
  if (std::fabs(d) < 1e-12) return false;
  double y[3];
  for (int k = 0; k < 3; ++k) {
    double t[3][3];
    for (int r = 0; r < 3; ++r)
      for (int c = 0; c < 3; ++c) t[r][c] = c == k ? w[r] : m[r][c];
    y[k] = (t[0][0] * (t[1][1] * t[2][2] - t[1][2] * t[2][1]) - t[0][1] * (t[1][0] * t[2][2] - t[1][2] * t[2][0]) + t[0][2] * (t[1][0] * t[2][1] - t[1][1] * t[2][0])) / d;
  }
  for (int k = 0; k < n; ++k) f[idx[k]] = a[0][k] * y[0] + a[1][k] * y[1] + a[2][k] * y[2];
  return true;
}

// Vertical forces on the feet that add to the weight and give the moments (mx, my) if every force can be zero or more; if not, the moments are scaled down (keeping their direction) to the largest that can be given, and false is returned.
inline bool foot_forces(const double feet[4][2], const bool on_ground[4], double W, double mx, double my, double f[4]) {
  if (!foot_forces_raw(feet, on_ground, W, mx, my, f)) return false;
  bool ok = true;
  for (int i = 0; i < 4; ++i)
    if (f[i] < 0.0) ok = false;
  if (ok) return true;
  double lo = 0.0, hi = 1.0;
  for (int it = 0; it < 50; ++it) {
    const double mid = 0.5 * (lo + hi);
    double g[4];
    foot_forces_raw(feet, on_ground, W, mid * mx, mid * my, g);
    bool good = true;
    for (int i = 0; i < 4; ++i)
      if (g[i] < 0.0) good = false;
    if (good) {
      lo = mid;
    } else {
      hi = mid;
    }
  }
  foot_forces_raw(feet, on_ground, W, lo * mx, lo * my, f);
  for (int i = 0; i < 4; ++i)
    if (f[i] < 0.0) f[i] = 0.0;
  return false;
}


#endif
