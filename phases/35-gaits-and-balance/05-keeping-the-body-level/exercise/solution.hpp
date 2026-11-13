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
  (void)W;  // TODO: for the n feet on the ground build the 3 by n matrix A with rows (1, y, -x), the wanted vector (W, mx, my), solve (A A^T) y = w (3 by 3, Cramer's rule; false if the determinant is nearly zero) and the force on foot k is the k-th entry of A^T y (it may be negative)
  (void)mx;
  (void)my;
  return false;
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
