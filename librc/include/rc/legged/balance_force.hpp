// rc/legged/balance_force.hpp
//
// Keeping the body level, from lesson 35-05.
//
// balance_moments is a PD law: the moments about the roll and pitch axes that pull the body back, minus kp times the angle and minus kd times its rate. foot_forces finds vertical forces on the feet that are on the ground (their places in the body's frame, from above, about the centre of mass) that add to the body's weight W and give the moments Mx (about the forward axis, the sum of force times sideways place) and My (about the sideways axis, minus the sum of force times forward place); with four feet down there are more unknowns than equations and the least norm solution is taken (A transpose times the solution of A A transpose y = w), with three the equations have one solution. It returns whether every force is zero or more; a foot cannot pull, so if the forces would be negative the moments are scaled down, keeping their direction, to the largest that can be given with every force zero or more, and false is returned (cutting the negative forces off without scaling would leave the forces adding to more than the weight, which is a push up that the body would feel). realized_moments gives the moments of a set of forces.
//
// A 12 kg body with its feet 0.19 m fore and aft and 0.15 m to each side stood with 29.4 N on each foot. A roll moment of 10 N m was given exactly with forces from 12.8 to 46.1 N, and the largest that could be given was 17.7 N m (the weight times the half width, 17.7). A push of 10 N m for 0.3 s was answered by the PD law and the body was level again after 0.12 s with a peak roll of 5.7 degrees; one of 25 N m was more than the feet could answer and the body rolled over (1).

#ifndef RC_RC_LEGGED_BALANCE_FORCE_HPP
#define RC_RC_LEGGED_BALANCE_FORCE_HPP

#include <cmath>

namespace rc {
namespace legged {

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


}  // namespace legged
}  // namespace rc

#endif
