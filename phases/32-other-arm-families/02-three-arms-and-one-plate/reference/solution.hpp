#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cmath>

struct Delta {
  double rb = 0.12;    // m, from the centre of the base to each motor axis
  double rp = 0.04;    // m, from the centre of the platform to each joint
  double rf = 0.20;    // m, the upper arm
  double re = 0.46;    // m, the lower arm (the parallelogram's rods)
  double th_lo = -0.7; // rad, the motor angle limits, measured down from the horizontal
  double th_hi = 1.4;
};

// One arm, in its own frame: the platform joint is at (x0, y0, z0) with z0 below the base; the angle the upper arm makes, down from the horizontal. False if the arm cannot reach.
inline bool delta_arm_angle(const Delta& d, double x0, double y0, double z0, double& theta) {
  const double y1 = -d.rb;
  const double yp = y0 - d.rp;
  if (std::fabs(z0) < 1e-9) return false;
  const double a = (x0 * x0 + yp * yp + z0 * z0 + d.rf * d.rf - d.re * d.re - y1 * y1) / (2.0 * z0);
  const double b = (y1 - yp) / z0;
  const double disc = -(a + b * y1) * (a + b * y1) + d.rf * d.rf * (b * b + 1.0);
  if (disc < 0.0) return false;
  const double yj = (y1 - a * b - std::sqrt(disc)) / (b * b + 1.0);
  const double zj = a + b * yj;
  theta = std::atan2(-zj, y1 - yj);
  return true;
}

// The three motor angles for the platform at (x, y, z), z below the base; false if any arm cannot reach or is outside its limits.
inline bool delta_ik(const Delta& d, double x, double y, double z, double th[3]) {
  const double cs[3] = {1.0, -0.5, -0.5};
  const double sn[3] = {0.0, 0.8660254037844386, -0.8660254037844386};
  for (int k = 0; k < 3; ++k) {
    const double xk = x * cs[k] + y * sn[k];
    const double yk = -x * sn[k] + y * cs[k];
    if (!delta_arm_angle(d, xk, yk, z, th[k])) return false;
    if (th[k] < d.th_lo || th[k] > d.th_hi) return false;
  }
  return true;
}

// The platform's place for three motor angles (three spheres meeting); false if they do not meet.
inline bool delta_fk(const Delta& d, const double th[3], double out[3]) {
  const double cs[3] = {1.0, -0.5, -0.5};
  const double sn[3] = {0.0, 0.8660254037844386, -0.8660254037844386};
  double c[3][3];
  for (int k = 0; k < 3; ++k) {
    // the centre of the sphere in arm k's frame, then turned back into the world
    const double yk = -d.rb - d.rf * std::cos(th[k]) + d.rp;
    const double zk = -d.rf * std::sin(th[k]);
    c[k][0] = 0.0 * cs[k] - yk * sn[k];
    c[k][1] = 0.0 * sn[k] + yk * cs[k];
    c[k][2] = zk;
  }
  double ex[3], t[3], ey[3], ez[3];
  double dist = 0.0;
  for (int i = 0; i < 3; ++i) {
    ex[i] = c[1][i] - c[0][i];
    dist += ex[i] * ex[i];
  }
  dist = std::sqrt(dist);
  if (dist < 1e-12) return false;
  for (int i = 0; i < 3; ++i) ex[i] /= dist;
  double ii = 0.0;
  for (int i = 0; i < 3; ++i) {
    t[i] = c[2][i] - c[0][i];
    ii += ex[i] * t[i];
  }
  double ln = 0.0;
  for (int i = 0; i < 3; ++i) {
    ey[i] = t[i] - ii * ex[i];
    ln += ey[i] * ey[i];
  }
  ln = std::sqrt(ln);
  if (ln < 1e-12) return false;
  for (int i = 0; i < 3; ++i) ey[i] /= ln;
  ez[0] = ex[1] * ey[2] - ex[2] * ey[1];
  ez[1] = ex[2] * ey[0] - ex[0] * ey[2];
  ez[2] = ex[0] * ey[1] - ex[1] * ey[0];
  double jj = 0.0;
  for (int i = 0; i < 3; ++i) jj += ey[i] * t[i];
  if (std::fabs(jj) < 1e-12) return false;
  const double x = dist / 2.0;
  const double y = (ii * ii + jj * jj) / (2.0 * jj) - ii / jj * x;
  const double z2 = d.re * d.re - x * x - y * y;
  if (z2 < 0.0) return false;
  const double z = std::sqrt(z2);
  double up[3], down[3];
  for (int i = 0; i < 3; ++i) {
    up[i] = c[0][i] + x * ex[i] + y * ey[i] + z * ez[i];
    down[i] = c[0][i] + x * ex[i] + y * ey[i] - z * ez[i];
  }
  const double* pick = up[2] < down[2] ? up : down;
  for (int i = 0; i < 3; ++i) out[i] = pick[i];
  return true;
}


#endif
