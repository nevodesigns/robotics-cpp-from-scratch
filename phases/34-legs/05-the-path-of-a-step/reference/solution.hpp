#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <algorithm>
#include <cmath>
#include <rc/legged/leg_kin.hpp>

using namespace rc::legged;

// The foot's place at a fraction tau (0 to 1) of the swing from p0 to p1, with a top h above the straight line between them.
inline void swing_position(const double p0[3], const double p1[3], double h, double tau, double out[3]) {
  const double t = tau < 0.0 ? 0.0 : (tau > 1.0 ? 1.0 : tau);
  const double s = t * t * t * (10.0 - 15.0 * t + 6.0 * t * t);
  for (int k = 0; k < 3; ++k) out[k] = p0[k] + (p1[k] - p0[k]) * s;
  out[2] += 64.0 * h * t * t * t * (1.0 - t) * (1.0 - t) * (1.0 - t);
}

// The foot's place in the body's frame a time t into a stance that began at td, with the body going at v (x, y).
inline void stance_position(const double td[3], const double v[2], double t, double out[3]) {
  out[0] = td[0] - v[0] * t;
  out[1] = td[1] - v[1] * t;
  out[2] = td[2];
}

// The peak speed and acceleration of the foot along a swing of duration t_sw, by stepping through it.
inline void swing_peaks(const double p0[3], const double p1[3], double h, double t_sw, double* v_peak, double* a_peak) {
  const int n = 400;
  double prev[3], prev_v[3] = {0, 0, 0};
  swing_position(p0, p1, h, 0.0, prev);
  double vp = 0.0, ap = 0.0;
  const double dt = t_sw / n;
  for (int i = 1; i <= n; ++i) {
    double p[3];
    swing_position(p0, p1, h, static_cast<double>(i) / n, p);
    double v[3], a[3];
    for (int k = 0; k < 3; ++k) {
      v[k] = (p[k] - prev[k]) / dt;
      a[k] = (v[k] - prev_v[k]) / dt;
    }
    const double sp = std::sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
    if (sp > vp) vp = sp;
    if (i > 1) {
      const double ac = std::sqrt(a[0] * a[0] + a[1] * a[1] + a[2] * a[2]);
      if (ac > ap) ap = ac;
    }
    for (int k = 0; k < 3; ++k) {
      prev[k] = p[k];
      prev_v[k] = v[k];
    }
  }
  *v_peak = vp;
  *a_peak = ap;
}

// The greatest joint rate (rad/s) any of the three joints needs along the swing (inverse kinematics at points of the path); false if a point cannot be reached.
inline bool swing_peak_joint_rate(const LegGeom& g, const double p0[3], const double p1[3], double h, double t_sw, int knee, double* peak) {
  const int n = 200;
  double prev[3];
  double worst = 0.0;
  const double dt = t_sw / n;
  for (int i = 0; i <= n; ++i) {
    double p[3], q[3];
    swing_position(p0, p1, h, static_cast<double>(i) / n, p);
    if (!leg_ik(g, p[0], p[1], p[2], knee, q)) return false;
    if (i > 0)
      for (int k = 0; k < 3; ++k) worst = std::max(worst, std::fabs(q[k] - prev[k]) / dt);
    for (int k = 0; k < 3; ++k) prev[k] = q[k];
  }
  *peak = worst;
  return true;
}


#endif
