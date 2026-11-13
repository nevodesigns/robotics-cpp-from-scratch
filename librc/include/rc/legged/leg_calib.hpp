// rc/legged/leg_calib.hpp
//
// Making the zeros true, from lesson 34-08.
//
// calibrate_offsets takes the angles commanded at n poses and the places the foot was measured at, and finds the offset of each of the three joints: the angle that has to be added to what is commanded for the leg to be where it is. Each pass predicts the foot's place for the commanded angles plus the offsets found so far, forms the difference from the measured place, and solves the normal equations (the sum over the poses of the Jacobian's transpose times its Jacobian, and times the difference) for a correction, until the correction is tiny. It gives false if the poses do not fix the offsets (the normal equations are singular, for example when the poses are all the same) and the rms of the differences that remain, which is what the offsets cannot explain.
//
// With 12 poses and 2 mm of noise in the measured places, offsets of 2, -3 and 4 degrees were found as 1.95, -3.16 and 4.19 degrees; the foot was 14.6 mm from where it was commanded before the fit and 3.4 mm after (the noise of 2 mm in each axis of the measurements is 3.5 mm in all three, which is what was left). Poses with the leg straight in every one gave no fit. With a thigh 10 mm too long the offsets could not explain it and the residual stayed at 3.6 mm.

#ifndef RC_RC_LEGGED_LEG_CALIB_HPP
#define RC_RC_LEGGED_LEG_CALIB_HPP

#include <cmath>
#include <rc/legged/leg_force.hpp>
#include <rc/legged/leg_kin.hpp>

namespace rc {
namespace legged {

// Solves a 3 by 3 system by Cramer's rule with a relative test for singularity (the determinant against the size of the matrix).
inline bool solve3_rel(const double m[3][3], const double b[3], double x[3]) {
  double scale = 0.0;
  for (int r = 0; r < 3; ++r)
    for (int c = 0; c < 3; ++c) scale = std::fmax(scale, std::fabs(m[r][c]));
  const double d = det3(m);
  if (scale <= 0.0 || std::fabs(d) < 1e-12 * scale * scale * scale) return false;
  for (int k = 0; k < 3; ++k) {
    double t[3][3];
    for (int r = 0; r < 3; ++r)
      for (int c = 0; c < 3; ++c) t[r][c] = c == k ? b[r] : m[r][c];
    x[k] = det3(t) / d;
  }
  return true;
}

// The offsets to add to the commanded angles of n poses so that the leg's foot is where it was measured; the rms of what remains in `rms`. False if the poses do not fix the offsets.
inline bool calibrate_offsets(const LegGeom& g, const double q[][3], const double p[][3], int n, int iters, double delta[3], double* rms) {
  delta[0] = delta[1] = delta[2] = 0.0;
  for (int it = 0; it < iters; ++it) {
    double a[3][3] = {{0, 0, 0}, {0, 0, 0}, {0, 0, 0}}, b[3] = {0, 0, 0};
    for (int i = 0; i < n; ++i) {
      const double qq[3] = {q[i][0] + delta[0], q[i][1] + delta[1], q[i][2] + delta[2]};
      double j[3][3];
      leg_jacobian(g, qq[0], qq[1], qq[2], j);
      const FootPos pred = leg_fk(g, qq[0], qq[1], qq[2]);
      const double r[3] = {p[i][0] - pred.x, p[i][1] - pred.y, p[i][2] - pred.z};
      for (int c = 0; c < 3; ++c) {
        for (int k = 0; k < 3; ++k)
          for (int row = 0; row < 3; ++row) a[c][k] += j[row][c] * j[row][k];
        for (int row = 0; row < 3; ++row) b[c] += j[row][c] * r[row];
      }
    }
    double d[3];
    if (!solve3_rel(a, b, d)) return false;
    double step = 0.0;
    for (int k = 0; k < 3; ++k) {
      delta[k] += d[k];
      step += d[k] * d[k];
    }
    if (std::sqrt(step) < 1e-12) break;
  }
  double sum = 0.0;
  for (int i = 0; i < n; ++i) {
    const FootPos pred = leg_fk(g, q[i][0] + delta[0], q[i][1] + delta[1], q[i][2] + delta[2]);
    sum += (p[i][0] - pred.x) * (p[i][0] - pred.x) + (p[i][1] - pred.y) * (p[i][1] - pred.y) + (p[i][2] - pred.z) * (p[i][2] - pred.z);
  }
  if (rms) *rms = std::sqrt(sum / (n > 0 ? n : 1));
  return true;
}


}  // namespace legged
}  // namespace rc

#endif
