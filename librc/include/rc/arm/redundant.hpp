// rc/arm/redundant.hpp
//
// One joint too many, from lesson 28-07.
//
// A six row Jacobian of a seven joint arm has a null space: joint motions that the tool does not feel. null_space_motion takes the part of a wanted joint motion that the tool does not feel: the motion less the damped solution for the tool velocity it would cause, m minus J transpose times (J J transpose plus lambda squared) inverse times J m. joint_margin is the smallest distance of any joint to a limit as a share of that joint's range; limit_gradient is the direction in which the squared distance of the joints from the middle of their limits grows, so that its negative steers them towards the middle. self_motion_run moves the joints by the null space motion towards the middle of the limits for some seconds, with the tool held in place by a correction on its error (when the gain is not zero), and reports how far the tool drifted, the margin before and after, and the distance the joints travelled.
//
// For random poses of the Panda a joint motion that went through the null space was felt by the tool at 0.0000000001 of its size; for the six joint UR5 the null space motion was 0.0000002311 of its size (there is none). A motion applied without the projection was felt as 1.244 m/s against 0.00000000 through it. With joint 3 started at 2.4 rad, 0.086 of its range from its limit, the self motion over 6 s left the tool within 0.00037 m, took the margin to 0.156 and moved the joints 0.66 rad; with no null space term the margin stayed 0.086; with the tool not corrected, the tool drifted 0.00076 m. A target that needed joint 6 at -0.3 rad, outside its limit, was reached by the unclamped iteration with joint 6 0.283 rad outside its limit; the clamped one stayed inside, ending with joint 6 at 0.013 rad.

#ifndef RC_RC_ARM_REDUNDANT_HPP
#define RC_RC_ARM_REDUNDANT_HPP

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <rc/arm/ik_dls.hpp>
#include <vector>

namespace rc {
namespace arm {

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


}  // namespace arm
}  // namespace rc

#endif
