// rc/arm/computed_torque.hpp
//
// Using the model to move, from lesson 29-05.
//
// quintic gives the position, velocity and acceleration of a move that starts and ends at rest with no jump in acceleration: s(t) = 10 u cubed minus 15 u to the fourth plus 6 u to the fifth, u = t over T, for each joint from its start to its end. computed_torque is the torque the model says gives the wanted acceleration plus feedback: the mass matrix times (the reference acceleration plus Kv times the velocity error plus Kp times the position error), plus the torques of the velocity products and of gravity. If the model is exact the error obeys e'' + Kv e' + Kp e = 0 in every joint separately. scaled_model multiplies every mass and inertia by a factor, which is a model that is wrong by that much. track runs the arm (the real one) under either controller (the joint PD with gravity of 29-02, or computed torque built on the model it believes) along a quintic move and gives the root mean square error, the largest error and the largest torque.
//
// For a move of 0.7 rad in a second the joint PD with gravity had an rms error of 0.04293 rad and computed torque with an exact model 0.000056948. From a start 0.1 rad out computed torque with a bandwidth of 20 rad/s crossed zero 0 times and was within 1 percent in 0.33 s. With a model wrong by 10, 20 and 40 percent in every mass the rms error was 0.00467, 0.00853 and 0.01455 rad, and with a model 40 percent too light and nothing in the gripper beyond it 0.03469. The largest torque was 28.5 N m for the joint PD and 28.5 for computed torque: the same, set by holding the arm out.

#ifndef RC_RC_ARM_COMPUTED_TORQUE_HPP
#define RC_RC_ARM_COMPUTED_TORQUE_HPP

#include <cmath>
#include <cstddef>
#include <rc/arm/gravity_comp.hpp>
#include <vector>

namespace rc {
namespace arm {

struct Quintic {
  std::vector<double> q0, q1;
  double duration = 1.0;
};

// Position, velocity and acceleration of the move at a time (at rest before and after).
inline void quintic_at(const Quintic& m, double t, std::vector<double>& q, std::vector<double>& qd, std::vector<double>& qdd) {
  const std::size_t n = m.q0.size();
  q.assign(n, 0.0);
  qd.assign(n, 0.0);
  qdd.assign(n, 0.0);
  double u = t / m.duration;
  u = u < 0.0 ? 0.0 : (u > 1.0 ? 1.0 : u);
  const double s = 10.0 * u * u * u - 15.0 * u * u * u * u + 6.0 * u * u * u * u * u;
  const bool moving = t > 0.0 && t < m.duration;
  const double ds = moving ? (30.0 * u * u - 60.0 * u * u * u + 30.0 * u * u * u * u) / m.duration : 0.0;
  const double dds = moving ? (60.0 * u - 180.0 * u * u + 120.0 * u * u * u) / (m.duration * m.duration) : 0.0;
  for (std::size_t i = 0; i < n; ++i) {
    const double d = m.q1[i] - m.q0[i];
    q[i] = m.q0[i] + d * s;
    qd[i] = d * ds;
    qdd[i] = d * dds;
  }
}

// The torque the model says gives the wanted acceleration, plus feedback on the position and velocity errors.
inline std::vector<double> computed_torque(const ArmModel& m, const std::vector<double>& q, const std::vector<double>& qd, const std::vector<double>& q_des, const std::vector<double>& qd_des,
                                           const std::vector<double>& qdd_des, double kp, double kv) {
  const std::size_t n = q.size();
  std::vector<double> want(n);
  for (std::size_t i = 0; i < n; ++i) want[i] = qdd_des[i] + kv * (qd_des[i] - qd[i]) + kp * (q_des[i] - q[i]);
  const double g[3] = {0.0, 0.0, -9.81};
  return inverse_dynamics(m, q, qd, want, g);
}

// A model whose every mass and inertia is the given factor times the model's.
inline ArmModel scaled_model(const ArmModel& m, double factor) {
  ArmModel out = m;
  for (std::size_t i = 0; i < out.links.size(); ++i) {
    out.links[i].mass *= factor;
    for (int r = 0; r < 3; ++r)
      for (int c = 0; c < 3; ++c) out.links[i].inertia[r][c] *= factor;
  }
  return out;
}

struct TrackResult {
  double rms_error = 0.0;   // rad
  double max_error = 0.0;   // rad
  double max_torque = 0.0;  // N m
};

// Runs the real arm along a move under a controller that believes a model. mode 0: joint PD with gravity (the gains are in `pd`); mode 1: computed torque with the bandwidth wn (kp = wn^2, kv = 2 wn).
inline TrackResult track(const ArmModel& real, const ArmModel& believed, const Quintic& move, int mode, const PdGains& pd, double wn, double dt, double seconds) {
  TrackResult out;
  std::vector<double> q = move.q0, qd(move.q0.size(), 0.0), qs, qds, qdds;
  const int steps = static_cast<int>(seconds / dt);
  double sum = 0.0;
  for (int k = 0; k < steps; ++k) {
    quintic_at(move, k * dt, qs, qds, qdds);
    std::vector<double> tau;
    if (mode == 0) {
      tau = pd_torque(believed, q, qd, qs, pd, true);
    } else {
      tau = computed_torque(believed, q, qd, qs, qds, qdds, wn * wn, 2.0 * wn);
    }
    for (std::size_t i = 0; i < tau.size(); ++i) out.max_torque = std::fmax(out.max_torque, std::fabs(tau[i]));
    double e2 = 0.0;
    for (std::size_t i = 0; i < q.size(); ++i) {
      const double e = qs[i] - q[i];
      e2 += e * e;
      out.max_error = std::fmax(out.max_error, std::fabs(e));
    }
    sum += e2 / static_cast<double>(q.size());
    step_dynamics(real, q, qd, tau, dt, true);
    if (!(out.max_error < 1e6)) {
      out.rms_error = out.max_error = 1e9;
      return out;
    }
  }
  out.rms_error = std::sqrt(sum / steps);
  return out;
}


}  // namespace arm
}  // namespace rc

#endif
