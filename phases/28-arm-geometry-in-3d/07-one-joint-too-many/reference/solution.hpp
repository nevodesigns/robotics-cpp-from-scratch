#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <rc/arm/ik_dls.hpp>
#include <vector>

using namespace rc::arm;

// The joint margin: the least distance of any joint to a limit, as a share of that joint's range (negative if a joint is outside).
inline double joint_margin(const ArmTable& t, const std::vector<double>& q) {
  double worst = 1e9;
  for (std::size_t i = 0; i < q.size(); ++i) {
    const double range = t.rows[i].hi - t.rows[i].lo;
    const double m = std::fmin(q[i] - t.rows[i].lo, t.rows[i].hi - q[i]) / range;
    worst = std::fmin(worst, m);
  }
  return worst;
}

// The gradient of H = sum of ((q - middle) / range)^2: the direction in which the joints get further from the middle of their limits.
inline std::vector<double> limit_gradient(const ArmTable& t, const std::vector<double>& q) {
  std::vector<double> g(q.size());
  for (std::size_t i = 0; i < q.size(); ++i) {
    const double range = t.rows[i].hi - t.rows[i].lo;
    const double mid = 0.5 * (t.rows[i].hi + t.rows[i].lo);
    g[i] = 2.0 * (q[i] - mid) / (range * range);
  }
  return g;
}

// The part of a joint motion that the tool does not feel: m minus the damped solution for the tool velocity it would cause.
inline std::vector<double> null_space_motion(const Jacobian& j, const std::vector<double>& m, double lambda) {
  std::vector<double> out = m;
  double v[6];
  for (int r = 0; r < 6; ++r) {
    v[r] = 0.0;
    for (std::size_t k = 0; k < j.n; ++k) v[r] += j.m[r][k] * m[k];
  }
  const std::vector<double> felt = dls_step(j, v, lambda);
  for (std::size_t k = 0; k < j.n; ++k) out[k] = m[k] - felt[k];
  return out;
}

struct SelfMotion {
  double tool_drift = 0.0;    // m, the largest distance of the tool from where it was held
  double margin_start = 0.0;
  double margin_end = 0.0;
  double joint_travel = 0.0;  // rad, the distance between the first and last joint angles
  std::vector<double> q_end;
};

// Holds the tool in place for some seconds and moves the joints, through the null space, towards the middle of their limits. kp is the gain on the tool's error (zero: no correction).
inline SelfMotion self_motion_run(const ArmTable& t, std::vector<double> q, const Pose& base, const Pose& tool, double k_null, double kp, double seconds, double dt, double lambda) {
  SelfMotion out;
  const std::vector<double> q0 = q;
  const Pose held = forward(t, q, base, tool);
  out.margin_start = joint_margin(t, q);
  const int steps = static_cast<int>(seconds / dt);
  for (int s = 0; s < steps; ++s) {
    const Pose now = forward(t, q, base, tool);
    double e[6];
    pose_error(held, now, e);
    out.tool_drift = std::fmax(out.tool_drift, std::sqrt(e[0] * e[0] + e[1] * e[1] + e[2] * e[2]));
    const Jacobian j = jacobian(t, q, base, tool, t.convention);
    double ec[6];
    for (int r = 0; r < 6; ++r) ec[r] = kp * e[r] * dt;
    const std::vector<double> back = dls_step(j, ec, lambda);
    std::vector<double> want = limit_gradient(t, q);
    for (std::size_t i = 0; i < want.size(); ++i) want[i] *= -k_null * dt;
    const std::vector<double> self = null_space_motion(j, want, lambda);
    for (std::size_t i = 0; i < q.size(); ++i) q[i] += back[i] + self[i];
  }
  out.margin_end = joint_margin(t, q);
  double n = 0.0;
  for (std::size_t i = 0; i < q.size(); ++i) n += (q[i] - q0[i]) * (q[i] - q0[i]);
  out.joint_travel = std::sqrt(n);
  out.q_end = q;
  return out;
}


#endif
