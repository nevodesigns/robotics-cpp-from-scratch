// rc/legged/foot_path.hpp
//
// The path of a step, from lesson 34-05.
//
// swing_position gives the foot's place at a fraction tau of the swing, from where it leaves (p0) to where it lands (p1) with a top at height h above the straight line between them: along the ground the foot moves by the minimum jerk curve 10 tau cubed less 15 tau to the fourth plus 6 tau to the fifth (zero speed and acceleration at both ends) and in height by 64 h tau cubed (1 - tau) cubed (zero speed and acceleration at both ends and h at the middle). stance_position is the foot's place in the body's frame a time into the stance. swing_peaks finds the peak speed and acceleration of the foot along the swing by stepping through it. swing_peak_joint_rate finds, by the inverse kinematics of 34-01 at points of the path and differences between them, the greatest rate any of the three joints needs, and whether every point could be reached.
//
// A step of 0.10 m, 0.05 m high in 0.25 s left the ground at a speed of 0.000 m/s and landed at 0.000 m/s, where a path that goes straight up and down at the same height landed at 0.80 m/s. The peak foot speed was 0.87 m/s and its peak acceleration 19.2 m/s2. It cleared an obstacle 0.04 m high in the middle of the step by 10 mm. The three joints needed up to 4.7 rad/s over a swing of 0.25 s and 9.4 rad/s over 0.125 s, against 8 rad/s the servos can give.

#ifndef RC_RC_LEGGED_FOOT_PATH_HPP
#define RC_RC_LEGGED_FOOT_PATH_HPP

#include <algorithm>
#include <cmath>
#include <rc/legged/leg_kin.hpp>

namespace rc {
namespace legged {

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


}  // namespace legged
}  // namespace rc

#endif
