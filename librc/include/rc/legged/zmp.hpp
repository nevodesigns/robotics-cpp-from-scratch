// rc/legged/zmp.hpp
//
// Where the weight presses, from lesson 35-06.
//
// cop_from_forces is the centre of pressure: the mean of the feet's places weighted by the vertical forces on them. zmp_from_com is the zero moment point of a body whose centre of mass is a height h above the ground and is accelerating horizontally by (ax, ay) while it does not turn: the centre of mass moved back by h over g times the acceleration, which is where the centre of pressure has to be for the body to be in balance. support_margin is how far inside the feet that are on the ground a point is (a polygon of the feet taken in order front left, front right, rear right, rear left for three or four feet; for two feet there is only a line, and the margin is minus the distance from it, which is never positive). max_accel is the greatest acceleration in a direction for which the point stays inside the feet, found by halving an interval.
//
// An acceleration of 3 m/s2 forward on a body 0.27 m high moved the point 83 mm behind the centre of mass, and the centre of pressure of the feet loaded to give it was back at 0.000000000 m from that point. With four feet down the body could accelerate forward at 6.90 m/s2, backward at 6.90 and sideways at 5.45, which are g times the distance to the edge over the height, and agree with the tipping limit of the arm lesson to 1; with a hind foot lifted and the body centred the margin was 0.000 m and the limit 0.00 m/s2; and with two diagonal feet down the margin was 0.000 m: no acceleration is safe.

#ifndef RC_RC_LEGGED_ZMP_HPP
#define RC_RC_LEGGED_ZMP_HPP

#include <cmath>
#include <rc/legged/static_gait.hpp>

namespace rc {
namespace legged {

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


}  // namespace legged
}  // namespace rc

#endif
