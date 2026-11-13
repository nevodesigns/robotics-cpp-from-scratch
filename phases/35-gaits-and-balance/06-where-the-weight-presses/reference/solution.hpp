#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cmath>
#include <rc/legged/static_gait.hpp>

using namespace rc::legged;

// The mean of the feet's places (x, y) weighted by the vertical forces on them.
inline bool cop_from_forces(const double feet[4][2], const double f[4], double out[2]) {
  double sum = 0.0;
  out[0] = out[1] = 0.0;
  for (int i = 0; i < 4; ++i) {
    sum += f[i];
    out[0] += f[i] * feet[i][0];
    out[1] += f[i] * feet[i][1];
  }
  if (sum <= 0.0) return false;
  out[0] /= sum;
  out[1] /= sum;
  return true;
}

// The zero moment point (x, y) of a body whose centre is at com, a height h up, accelerating horizontally by a (not turning).
inline void zmp_from_com(const double com[2], double h, const double a[2], double out[2]) {
  out[0] = com[0] - h / 9.81 * a[0];
  out[1] = com[1] - h / 9.81 * a[1];
}

// How far inside the feet on the ground a point is: the polygon's margin for three or four feet (taken in the order front left, front right, rear right, rear left), minus the distance from the line for two, and minus one for fewer.
inline double support_margin(const double feet[4][2], const bool on_ground[4], const double p[2]) {
  const int order[4] = {0, 1, 3, 2};
  double pts[4][2];
  int n = 0;
  for (int i = 0; i < 4; ++i)
    if (on_ground[order[i]]) {
      pts[n][0] = feet[order[i]][0];
      pts[n][1] = feet[order[i]][1];
      ++n;
    }
  if (n >= 3) return polygon_margin(pts, n, p);
  if (n == 2) {
    const double ex = pts[1][0] - pts[0][0], ey = pts[1][1] - pts[0][1];
    const double len = std::sqrt(ex * ex + ey * ey);
    return -std::fabs(ex * (p[1] - pts[0][1]) - ey * (p[0] - pts[0][0])) / len;
  }
  return -1.0;
}

// The greatest acceleration along the unit direction u (x, y) that keeps the zero moment point inside the feet; zero if it is not inside to begin with.
inline double max_accel(const double feet[4][2], const bool on_ground[4], const double com[2], double h, const double u[2]) {
  if (support_margin(feet, on_ground, com) < 0.0) return 0.0;
  double lo = 0.0, hi = 100.0;
  for (int i = 0; i < 60; ++i) {
    const double mid = 0.5 * (lo + hi);
    const double a[2] = {mid * u[0], mid * u[1]};
    double z[2];
    zmp_from_com(com, h, a, z);
    if (support_margin(feet, on_ground, z) >= 0.0) {
      lo = mid;
    } else {
      hi = mid;
    }
  }
  return lo;
}


#endif
