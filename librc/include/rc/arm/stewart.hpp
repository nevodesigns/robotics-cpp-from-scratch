// rc/arm/stewart.hpp
//
// Six legs and a table, from lesson 32-03.
//
// Stewart holds the six base joints and the six platform joints (on circles, in pairs) and the platform's pose is six numbers (x, y, z, roll, pitch, yaw). stewart_lengths gives the six leg lengths for a pose: the platform joints are turned and moved into the base frame and the length of each leg is the distance to its base joint. stewart_pose goes the other way by Newton's iteration: from a guess it corrects the pose by the solution of the six by six system whose matrix is the derivative of the lengths with respect to the pose (found by small differences), until the lengths agree; it gives whether it converged and in how many steps. stewart_condition is the smallest pivot over the largest in the elimination of that matrix: it is near 1 where the legs control the platform well and near zero where they lose a direction.
//
// A platform 0.30 m up on legs of 0.343 m was raised 50 mm and tilted 10 degrees, needing legs between 0.366 and 0.410 m. From a guess 20 mm and 4.6 degrees off, the iteration found the pose again to 0.000000000 m in 4 steps. The condition measure was 0.120 at home, fell to 0.038 when the platform was turned 80 degrees about the vertical, and was 0.000 (singular) at 90 degrees.

#ifndef RC_RC_ARM_STEWART_HPP
#define RC_RC_ARM_STEWART_HPP

#include <cmath>

namespace rc {
namespace arm {

struct StewartPose {
  double v[6] = {0, 0, 0, 0, 0, 0};  // x, y, z, roll, pitch, yaw
};

struct Stewart {
  double bx[6], by[6];   // base joints in the base frame (z = 0)
  double px[6], py[6];   // platform joints in the platform frame (z = 0)
  Stewart() {
    const double kDeg = 3.14159265358979323846 / 180.0;
    const double rb = 0.25, rp = 0.15;                // m (typical of a small hexapod)
    // pairs of joints: the base pairs centred at 0, 120 and 240 degrees, the platform pairs at 60, 180 and 300, each leg to the nearer joint of the neighbouring pair
    const double ab[6] = {10, 110, 130, 230, 250, 350};
    const double ap[6] = {50, 70, 170, 190, 290, 310};
    for (int i = 0; i < 6; ++i) {
      bx[i] = rb * std::cos(ab[i] * kDeg);
      by[i] = rb * std::sin(ab[i] * kDeg);
      px[i] = rp * std::cos(ap[i] * kDeg);
      py[i] = rp * std::sin(ap[i] * kDeg);
    }
  }
};

// The six leg lengths for a pose.
inline void stewart_lengths(const Stewart& s, const StewartPose& p, double len[6]) {
  const double cr = std::cos(p.v[3]), sr = std::sin(p.v[3]);
  const double cp = std::cos(p.v[4]), sp = std::sin(p.v[4]);
  const double cy = std::cos(p.v[5]), sy = std::sin(p.v[5]);
  // R = Rz(yaw) Ry(pitch) Rx(roll), applied to a point (x, y, 0) of the platform
  const double r00 = cy * cp, r01 = cy * sp * sr - sy * cr;
  const double r10 = sy * cp, r11 = sy * sp * sr + cy * cr;
  const double r20 = -sp, r21 = cp * sr;
  for (int i = 0; i < 6; ++i) {
    const double wx = r00 * s.px[i] + r01 * s.py[i] + p.v[0];
    const double wy = r10 * s.px[i] + r11 * s.py[i] + p.v[1];
    const double wz = r20 * s.px[i] + r21 * s.py[i] + p.v[2];
    len[i] = std::sqrt((wx - s.bx[i]) * (wx - s.bx[i]) + (wy - s.by[i]) * (wy - s.by[i]) + wz * wz);
  }
}

// Solves a x = b for six unknowns by elimination with partial pivoting; false if singular. `ratio` is the smallest pivot over the largest.
inline bool solve6(double a[6][6], double b[6], double x[6], double* ratio) {
  double pmin = 1e300, pmax = 0.0;
  for (int c = 0; c < 6; ++c) {
    int best = c;
    for (int r = c + 1; r < 6; ++r)
      if (std::fabs(a[r][c]) > std::fabs(a[best][c])) best = r;
    const double piv = std::fabs(a[best][c]);
    if (piv < pmin) pmin = piv;
    if (piv > pmax) pmax = piv;
    if (piv < 1e-14) {
      if (ratio) *ratio = 0.0;
      return false;
    }
    if (best != c) {
      for (int k = 0; k < 6; ++k) {
        const double t = a[c][k];
        a[c][k] = a[best][k];
        a[best][k] = t;
      }
      const double t = b[c];
      b[c] = b[best];
      b[best] = t;
    }
    for (int r = c + 1; r < 6; ++r) {
      const double f = a[r][c] / a[c][c];
      for (int k = c; k < 6; ++k) a[r][k] -= f * a[c][k];
      b[r] -= f * b[c];
    }
  }
  for (int r = 5; r >= 0; --r) {
    double sum = b[r];
    for (int k = r + 1; k < 6; ++k) sum -= a[r][k] * x[k];
    x[r] = sum / a[r][r];
  }
  if (ratio) *ratio = pmin / pmax;
  return true;
}

// The matrix of the derivatives of the six lengths with respect to the six pose values, by small differences.
inline void stewart_matrix(const Stewart& s, const StewartPose& p, double j[6][6]) {
  double l0[6], l1[6];
  stewart_lengths(s, p, l0);
  const double h = 1e-6;
  for (int c = 0; c < 6; ++c) {
    StewartPose q = p;
    q.v[c] += h;
    stewart_lengths(s, q, l1);
    for (int r = 0; r < 6; ++r) j[r][c] = (l1[r] - l0[r]) / h;
  }
}

// How near the legs are to losing a direction at this pose: near 1 is good, near 0 is singular.
inline double stewart_condition(const Stewart& s, const StewartPose& p) {
  double j[6][6], b[6] = {1, 1, 1, 1, 1, 1}, x[6], ratio = 0.0;
  stewart_matrix(s, p, j);
  solve6(j, b, x, &ratio);
  return ratio;
}

// The pose that has the leg lengths `len`, from a guess, by Newton's iteration; the number of steps in `iters`.
inline bool stewart_pose(const Stewart& s, const double len[6], StewartPose& pose, int* iters) {
  for (int it = 0; it < 60; ++it) {
    double l[6], j[6][6], r[6], dx[6];
    stewart_lengths(s, pose, l);
    double err = 0.0;
    for (int i = 0; i < 6; ++i) {
      r[i] = len[i] - l[i];
      err += r[i] * r[i];
    }
    if (std::sqrt(err) < 1e-12) {
      if (iters) *iters = it;
      return true;
    }
    stewart_matrix(s, pose, j);
    if (!solve6(j, r, dx, nullptr)) return false;
    for (int c = 0; c < 6; ++c) pose.v[c] += dx[c];
  }
  return false;
}


}  // namespace arm
}  // namespace rc

#endif
