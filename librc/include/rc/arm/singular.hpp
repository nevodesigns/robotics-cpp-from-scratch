// rc/arm/singular.hpp
//
// Where the arm loses a direction, from lesson 28-06.
//
// singular_values gives the singular values of a Jacobian, largest first, as the square roots of the eigenvalues of J times J transpose, found by rotating away the off diagonal terms of the 6 by 6 matrix (Jacobi's method). The smallest is how far the arm is from a singularity, in the units of the Jacobian; manipulability is the product of them all and condition_number the ratio of the largest to the smallest. adaptive_lambda gives the damping to use for the smallest singular value: none when it is above a threshold, rising to a maximum as it falls to zero. resolved_rate_line runs the tool along a straight line with its orientation fixed, steering with the damped step and a correction on the position error, and gives the largest joint rate and the largest distance from the line.
//
// For the typical arm with a spherical wrist the smallest singular value was 0.172 in a generic pose and 0.00000000 with the wrist straight, 0.00000000 with the elbow stretched and 0.00000000 with the wrist centre over the shoulder; as joint 5 went from 0.5 to 0.1 to 0.01 rad it fell from 0.1337 to 0.0313 to 0.00317. Drawing a line at 0.25 m/s that passed 0.01 m from the shoulder axis, an undamped step turned joint 1 at 26.1 rad/s and a damped one at 2.0 rad/s, the damped one leaving the tool as much as 0.0448 m off the line against 0.0006 m.

#ifndef RC_RC_ARM_SINGULAR_HPP
#define RC_RC_ARM_SINGULAR_HPP

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <functional>
#include <rc/arm/ik_dls.hpp>
#include <vector>

namespace rc {
namespace arm {

// The singular values of a Jacobian, largest first: the square roots of the eigenvalues of J J^T (Jacobi's method on the 6 by 6).
inline void singular_values(const Jacobian& j, double s[6]) {
  double a[6][6];
  for (int r = 0; r < 6; ++r)
    for (int c = 0; c < 6; ++c) {
      a[r][c] = 0.0;
      for (std::size_t k = 0; k < j.n; ++k) a[r][c] += j.m[r][k] * j.m[c][k];
    }
  for (int sweep = 0; sweep < 60; ++sweep) {
    double off = 0.0, all = 0.0;
    for (int r = 0; r < 6; ++r)
      for (int c = 0; c < 6; ++c) {
        all += a[r][c] * a[r][c];
        if (r != c) off += a[r][c] * a[r][c];
      }
    if (off <= 1e-30 * (all + 1e-300)) break;
    for (int p = 0; p < 5; ++p)
      for (int q = p + 1; q < 6; ++q) {
        if (std::fabs(a[p][q]) < 1e-300) continue;
        const double theta = (a[q][q] - a[p][p]) / (2.0 * a[p][q]);
        const double t = (theta >= 0.0 ? 1.0 : -1.0) / (std::fabs(theta) + std::sqrt(theta * theta + 1.0));
        const double c = 1.0 / std::sqrt(t * t + 1.0), sn = t * c;
        for (int k = 0; k < 6; ++k) {
          const double akp = a[k][p], akq = a[k][q];
          a[k][p] = c * akp - sn * akq;
          a[k][q] = sn * akp + c * akq;
        }
        for (int k = 0; k < 6; ++k) {
          const double apk = a[p][k], aqk = a[q][k];
          a[p][k] = c * apk - sn * aqk;
          a[q][k] = sn * apk + c * aqk;
        }
      }
  }
  for (int i = 0; i < 6; ++i) s[i] = std::sqrt(std::fmax(0.0, a[i][i]));
  std::sort(s, s + 6, std::greater<double>());
}

inline double smallest_singular(const Jacobian& j) {
  double s[6];
  singular_values(j, s);
  return s[5];
}

inline double manipulability(const Jacobian& j) {
  double s[6];
  singular_values(j, s);
  double p = 1.0;
  for (int i = 0; i < 6; ++i) p *= s[i];
  return p;
}

inline double condition_number(const Jacobian& j) {
  double s[6];
  singular_values(j, s);
  return s[5] > 1e-300 ? s[0] / s[5] : 1e300;
}

// The damping for a smallest singular value: none above eps, rising to lambda_max as it falls to zero.
inline double adaptive_lambda(double sigma_min, double eps, double lambda_max) {
  if (!(sigma_min < eps)) return 0.0;
  const double r = sigma_min / eps;
  return lambda_max * std::sqrt(1.0 - r * r);
}

struct RateRun {
  double peak_rate = 0.0;     // rad/s, the largest joint rate in the run
  double max_off_line = 0.0;  // m, the largest distance of the tool from the line
};

struct RateConfig {
  bool adaptive = false;
  double lambda_fixed = 1e-6;
  double eps = 0.1;
  double lambda_max = 0.1;
  double kp = 5.0;      // 1/s, the correction of the position error
  double dt = 0.002;
};

// Moves the tool along a line from p0 in direction dir (a unit vector) at a speed for a length, orientation fixed, by resolved rates.
inline RateRun resolved_rate_line(const ArmTable& t, std::vector<double> q, const Pose& base, const Pose& tool, const double p0[3], const double dir[3], double speed,
                                  double length, const RateConfig& cfg) {
  RateRun out;
  const Pose start = forward(t, q, base, tool);
  const int steps = static_cast<int>(length / speed / cfg.dt);
  for (int k = 0; k < steps; ++k) {
    const double s = speed * cfg.dt * k;
    const Pose now = forward(t, q, base, tool);
    double want[3];
    for (int i = 0; i < 3; ++i) want[i] = p0[i] + dir[i] * s;
    double e[6];
    pose_error(start, now, e);  // the rotation rows: back to the start's orientation
    for (int i = 0; i < 3; ++i) e[i] = want[i] - now.p[i];  // the place rows: back to the line
    out.max_off_line = std::fmax(out.max_off_line, std::sqrt(e[0] * e[0] + e[1] * e[1] + e[2] * e[2]));
    double v[6];
    for (int i = 0; i < 3; ++i) v[i] = dir[i] * speed + cfg.kp * e[i];
    for (int i = 3; i < 6; ++i) v[i] = cfg.kp * e[i];
    const Jacobian j = jacobian(t, q, base, tool, t.convention);
    const double lambda = cfg.adaptive ? adaptive_lambda(smallest_singular(j), cfg.eps, cfg.lambda_max) : cfg.lambda_fixed;
    double ev[6];
    for (int i = 0; i < 6; ++i) ev[i] = v[i] * cfg.dt;
    const std::vector<double> dq = dls_step(j, ev, lambda);
    for (std::size_t i = 0; i < dq.size(); ++i) {
      out.peak_rate = std::fmax(out.peak_rate, std::fabs(dq[i]) / cfg.dt);
      q[i] += dq[i];
    }
  }
  return out;
}


}  // namespace arm
}  // namespace rc

#endif
