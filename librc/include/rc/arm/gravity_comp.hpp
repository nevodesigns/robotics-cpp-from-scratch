// rc/arm/gravity_comp.hpp
//
// Holding the arm up, from lesson 29-02.
//
// pd_torque is the torque of a joint controller with a gain on the position error and one on the velocity, and, if asked, the torque of gravity of the model for the present pose added on. with_payload gives a model whose last link carries a point mass at a place in its frame: the mass is the sum, the centre of mass their weighted mean and the inertia the two about it, by the parallel axis rule. payload_torque is the torque that holding a payload at the tool asks, the transpose of the Jacobian times its weight. estimate_payload takes the torques an arm needed to hold still in several poses (less what the arm alone needs) and finds the one mass that best explains them, by least squares. settle runs the arm and the controller from rest to a target for some seconds and gives the error that is left.
//
// With a gain of 300 N m/rad at the shoulder the arm, sagging under its own weight, ended 0.1113 rad from where it was told to go, and the torque of gravity over the gain said 0.1093; with the torque of gravity added the error was 0.000000000 rad. A payload of 2 kg that the controller did not know about left an error of 0.0646 rad; its mass was estimated from six poses of torques with 0.1 N m of noise as 2.00 kg, and with that in the model the error was 0.00010 rad.

#ifndef RC_RC_ARM_GRAVITY_COMP_HPP
#define RC_RC_ARM_GRAVITY_COMP_HPP

#include <cmath>
#include <cstddef>
#include <rc/arm/dynamics3d.hpp>
#include <rc/arm/jacobian.hpp>
#include <vector>

namespace rc {
namespace arm {

struct PdGains {
  std::vector<double> kp;  // N m per rad
  std::vector<double> kd;  // N m s per rad
};

// The torque of a joint controller: gain on the position error, gain on the velocity, and (if asked) the torque of gravity of the model for the pose.
inline std::vector<double> pd_torque(const ArmModel& model, const std::vector<double>& q, const std::vector<double>& qd, const std::vector<double>& target, const PdGains& g, bool compensate) {
  const std::size_t n = q.size();
  std::vector<double> tau(n);
  for (std::size_t i = 0; i < n; ++i) tau[i] = g.kp[i] * (target[i] - q[i]) - g.kd[i] * qd[i];
  if (compensate) {
    const std::vector<double> tg = gravity_torques(model, q);
    for (std::size_t i = 0; i < n; ++i) tau[i] += tg[i];
  }
  return tau;
}

// A model whose last link carries a point mass at `where` (in that link's frame).
inline ArmModel with_payload(const ArmModel& m, double mass, const double where[3]) {
  ArmModel out = m;
  LinkInertia& l = out.links.back();
  const double total = l.mass + mass;
  double com[3], d1[3], d2[3];
  for (int k = 0; k < 3; ++k) com[k] = (l.mass * l.com[k] + mass * where[k]) / total;
  for (int k = 0; k < 3; ++k) {
    d1[k] = l.com[k] - com[k];
    d2[k] = where[k] - com[k];
  }
  const double s1 = d1[0] * d1[0] + d1[1] * d1[1] + d1[2] * d1[2], s2 = d2[0] * d2[0] + d2[1] * d2[1] + d2[2] * d2[2];
  for (int r = 0; r < 3; ++r)
    for (int c = 0; c < 3; ++c) l.inertia[r][c] += l.mass * ((r == c ? s1 : 0.0) - d1[r] * d1[c]) + mass * ((r == c ? s2 : 0.0) - d2[r] * d2[c]);
  l.mass = total;
  for (int k = 0; k < 3; ++k) l.com[k] = com[k];
  return out;
}

// The torque it takes to hold a payload of a mass at the tool, in a pose: J^T times its weight (upward).
inline std::vector<double> payload_torque(const ArmTable& t, const std::vector<double>& q, const Pose& tool, double mass) {
  const Jacobian j = jacobian(t, q, Pose(), tool, t.convention);
  std::vector<double> tau(j.n, 0.0);
  for (std::size_t i = 0; i < j.n; ++i) tau[i] = j.m[2][i] * mass * 9.81;
  return tau;
}

// The one mass that best explains the extra torques needed to hold still in several poses (least squares).
inline double estimate_payload(const ArmTable& t, const Pose& tool, const std::vector<std::vector<double> >& poses, const std::vector<std::vector<double> >& extra_torques) {
  double num = 0.0, den = 0.0;
  for (std::size_t k = 0; k < poses.size(); ++k) {
    const std::vector<double> per_kg = payload_torque(t, poses[k], tool, 1.0);
    for (std::size_t i = 0; i < per_kg.size(); ++i) {
      num += per_kg[i] * extra_torques[k][i];
      den += per_kg[i] * per_kg[i];
    }
  }
  return den > 0.0 ? num / den : 0.0;
}

// Runs the controller on the arm (the real one: `real`) from rest at q0 to a target for some seconds; the controller sees `believed`. Returns the largest joint error at the end.
inline double settle(const ArmModel& real, const ArmModel& believed, const std::vector<double>& q0, const std::vector<double>& target, const PdGains& g, bool compensate, double seconds, double dt) {
  std::vector<double> q = q0, qd(q0.size(), 0.0);
  const int steps = static_cast<int>(seconds / dt);
  for (int s = 0; s < steps; ++s) {
    const std::vector<double> tau = pd_torque(believed, q, qd, target, g, compensate);
    step_dynamics(real, q, qd, tau, dt, true);
  }
  double worst = 0.0;
  for (std::size_t i = 0; i < q.size(); ++i) {
    const double e = std::fabs(target[i] - q[i]);
    if (!(e <= 1e9)) return 1e9;  // the run blew up
    worst = std::fmax(worst, e);
  }
  return worst;
}


}  // namespace arm
}  // namespace rc

#endif
