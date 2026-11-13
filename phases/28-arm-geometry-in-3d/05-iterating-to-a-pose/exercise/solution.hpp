#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <rc/arm/jacobian.hpp>
#include <vector>

using namespace rc::arm;

// The rotation as an axis times an angle.
inline void rotation_vector(const Pose& r, double w[3]) {
  const double c = std::fmax(-1.0, std::fmin(1.0, (r.r[0][0] + r.r[1][1] + r.r[2][2] - 1.0) / 2.0));
  const double v[3] = {0.5 * (r.r[2][1] - r.r[1][2]), 0.5 * (r.r[0][2] - r.r[2][0]), 0.5 * (r.r[1][0] - r.r[0][1])};
  const double s = std::sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
  const double theta = std::atan2(s, c);
  if (s > 1e-9) {
    for (int k = 0; k < 3; ++k) w[k] = v[k] * theta / s;
  } else if (c > 0.0) {
    for (int k = 0; k < 3; ++k) w[k] = v[k];
  } else {
    // A half turn: the axis is in the columns of (R + I) / 2.
    int best = 0;
    for (int k = 1; k < 3; ++k)
      if (r.r[k][k] > r.r[best][best]) best = k;
    double axis[3];
    for (int k = 0; k < 3; ++k) axis[k] = 0.5 * (r.r[k][best] + (k == best ? 1.0 : 0.0));
    const double n = std::sqrt(axis[0] * axis[0] + axis[1] * axis[1] + axis[2] * axis[2]);
    for (int k = 0; k < 3; ++k) w[k] = axis[k] / n * kPi;
  }
}

// How far a pose is from its target: three of position, three of rotation.
inline void pose_error(const Pose& target, const Pose& now, double e[6]) {
  for (int k = 0; k < 3; ++k) e[k] = target.p[k] - now.p[k];
  Pose rt = now;
  // transpose of the present rotation
  for (int i = 0; i < 3; ++i)
    for (int j = 0; j < 3; ++j) rt.r[i][j] = now.r[j][i];
  Pose d;
  for (int i = 0; i < 3; ++i)
    for (int j = 0; j < 3; ++j) {
      d.r[i][j] = 0.0;
      for (int k = 0; k < 3; ++k) d.r[i][j] += target.r[i][k] * rt.r[k][j];
    }
  rotation_vector(d, e + 3);
}

// Solves a 6 by 6 system by elimination with partial pivoting. False if it is singular to working precision.
inline bool solve6(double a[6][6], double b[6], double x[6]) {
  for (int c = 0; c < 6; ++c) {
    int p = c;
    for (int r = c + 1; r < 6; ++r)
      if (std::fabs(a[r][c]) > std::fabs(a[p][c])) p = r;
    if (std::fabs(a[p][c]) < 1e-300) return false;
    if (p != c) {
      for (int k = 0; k < 6; ++k) std::swap(a[c][k], a[p][k]);
      std::swap(b[c], b[p]);
    }
    for (int r = c + 1; r < 6; ++r) {
      const double f = a[r][c] / a[c][c];
      for (int k = c; k < 6; ++k) a[r][k] -= f * a[c][k];
      b[r] -= f * b[c];
    }
  }
  for (int r = 5; r >= 0; --r) {
    double s = b[r];
    for (int k = r + 1; k < 6; ++k) s -= a[r][k] * x[k];
    x[r] = s / a[r][r];
  }
  return true;
}

// The joint step for an error: J^T (J J^T + lambda^2 I)^-1 e. Rows of the error and the Jacobian are as they come (weights are the caller's).
inline std::vector<double> dls_step(const Jacobian& j, const double e[6], double lambda) {
  std::vector<double> dq(j.n, 0.0);
  double a[6][6], b[6], y[6];
  (void)e;  // TODO: A = J J^T + lambda^2 I (6 by 6); solve A y = e with solve6; the step is J^T y
  (void)lambda;
  (void)a;
  return dq;
}

struct DlsConfig {
  double lambda = 0.05;
  double rot_weight = 0.5;   // metres that a radian of rotation error counts as
  double tol_pos = 1e-6;     // m
  double tol_rot = 1e-6;     // rad
  double max_step = 0.3;     // rad, the largest change of any joint in one step
  int max_iter = 200;
  bool clamp = false;        // keep the joints inside their limits
};

struct IkResult {
  bool ok = false;
  std::vector<double> q;
  int iterations = 0;
  double pos_err = 0.0;
  double rot_err = 0.0;
};

inline IkResult ik_dls(const ArmTable& t, const Pose& target, const Pose& base, const Pose& tool, const std::vector<double>& q0, const DlsConfig& cfg) {
  IkResult out;
  out.q = q0;
  for (int it = 0; it <= cfg.max_iter; ++it) {
    const Pose now = forward(t, out.q, base, tool);
    double e[6];
    pose_error(target, now, e);
    out.pos_err = std::sqrt(e[0] * e[0] + e[1] * e[1] + e[2] * e[2]);
    out.rot_err = std::sqrt(e[3] * e[3] + e[4] * e[4] + e[5] * e[5]);
    out.iterations = it;
    if (out.pos_err < cfg.tol_pos && out.rot_err < cfg.tol_rot) {
      out.ok = true;
      return out;
    }
    if (it == cfg.max_iter) break;
    Jacobian j = jacobian(t, out.q, base, tool, t.convention);
    double ew[6];
    for (int r = 0; r < 6; ++r) {
      const double w = r < 3 ? 1.0 : cfg.rot_weight;
      ew[r] = e[r] * w;
      for (std::size_t c = 0; c < j.n; ++c) j.m[r][c] *= w;
    }
    std::vector<double> dq = dls_step(j, ew, cfg.lambda);
    double biggest = 0.0;
    for (std::size_t i = 0; i < dq.size(); ++i) biggest = std::fmax(biggest, std::fabs(dq[i]));
    const double scale = biggest > cfg.max_step ? cfg.max_step / biggest : 1.0;
    for (std::size_t i = 0; i < dq.size(); ++i) {
      out.q[i] += dq[i] * scale;
      if (cfg.clamp) out.q[i] = std::fmax(t.rows[i].lo, std::fmin(t.rows[i].hi, out.q[i]));
    }
  }
  return out;
}


#endif
