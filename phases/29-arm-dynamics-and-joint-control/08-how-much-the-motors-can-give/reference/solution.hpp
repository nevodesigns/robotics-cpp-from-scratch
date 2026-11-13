#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cmath>
#include <cstddef>
#include <rc/arm/computed_torque.hpp>
#include <vector>

using namespace rc::arm;

struct JointMotor {
  double kt_out = 4.0;       // N m per ampere at the output shaft (typical of a geared joint)
  double resistance = 1.0;   // ohm
  double r_th = 1.2;         // K per W
  double c_th = 200.0;       // J per K
  double rise_limit = 75.0;  // K above the air
  double tau_peak = 150.0;   // N m
  double tau_cont = 60.0;    // N m
};

// The heat in the windings for a torque held, in watts.
inline double heat_watts(const JointMotor& m, double torque) {
  const double i = torque / m.kt_out;
  return i * i * m.resistance;
}

// Seconds for a torque held to bring the winding from cold to its limit, in closed form; a very large number if it never does.
inline double time_to_limit(const JointMotor& m, double torque) {
  const double steady = heat_watts(m, torque) * m.r_th;
  if (steady <= m.rise_limit) return 1.0e18;
  return -m.r_th * m.c_th * std::log(1.0 - m.rise_limit / steady);
}

// The largest payload mass at the tool that every joint can hold at a pose, given each joint's torque limit and the rated payload of the structure; negative if even the arm alone is too heavy.
inline double max_payload(const ArmModel& m, const std::vector<double>& q, const Pose& tool, const std::vector<double>& limits, double rated) {
  const std::vector<double> own = gravity_torques(m, q);
  const std::vector<double> per_kg = payload_torque(m.table, q, tool, 1.0);
  double lo = 0.0, hi = rated;  // the structure's own rating caps what the motors could lift
  for (std::size_t i = 0; i < q.size(); ++i) {
    // |own + mass * per_kg| <= limit  gives an interval for the mass.
    if (std::fabs(per_kg[i]) < 1e-12) {
      if (std::fabs(own[i]) > limits[i]) return -1.0;
      continue;
    }
    const double a = (-limits[i] - own[i]) / per_kg[i], b = (limits[i] - own[i]) / per_kg[i];
    lo = std::fmax(lo, std::fmin(a, b));
    hi = std::fmin(hi, std::fmax(a, b));
  }
  if (hi < lo) return -1.0;
  return hi;
}

struct MoveTorques {
  std::vector<double> peak;  // N m, the largest absolute torque of each joint
  std::vector<double> rms;   // N m, the root mean square torque of each joint
};

// The torques of a quintic move from the model (no friction, no payload beyond the model), sampled 200 times.
inline MoveTorques move_torques(const ArmModel& m, const Quintic& mv) {
  const std::size_t n = mv.q0.size();
  MoveTorques out;
  out.peak.assign(n, 0.0);
  out.rms.assign(n, 0.0);
  const double g[3] = {0.0, 0.0, -9.81};
  const int samples = 200;
  std::vector<double> q, qd, qdd;
  for (int k = 0; k <= samples; ++k) {
    quintic_at(mv, mv.duration * k / samples, q, qd, qdd);
    const std::vector<double> tau = inverse_dynamics(m, q, qd, qdd, g);
    for (std::size_t i = 0; i < n; ++i) {
      out.peak[i] = std::fmax(out.peak[i], std::fabs(tau[i]));
      out.rms[i] += tau[i] * tau[i];
    }
  }
  for (std::size_t i = 0; i < n; ++i) out.rms[i] = std::sqrt(out.rms[i] / (samples + 1));
  return out;
}

// True if every joint's peak is within its peak limit and its rms within its continuous limit.
inline bool within_motors(const MoveTorques& t, const std::vector<JointMotor>& motors) {
  for (std::size_t i = 0; i < t.peak.size(); ++i)
    if (!(t.peak[i] <= motors[i].tau_peak && t.rms[i] <= motors[i].tau_cont)) return false;
  return true;
}

// The shortest duration of the move for which the motors can do it (by halving between a very short and a very long one), or a negative number if even a very slow move is too much (the weight alone).
inline double shortest_move_time(const ArmModel& m, Quintic mv, const std::vector<JointMotor>& motors, bool peak_only) {
  std::vector<JointMotor> use = motors;
  if (peak_only)
    for (std::size_t i = 0; i < use.size(); ++i) use[i].tau_cont = 1e9;
  mv.duration = 60.0;
  if (!within_motors(move_torques(m, mv), use)) return -1.0;
  double lo = 0.05, hi = 60.0;
  for (int k = 0; k < 40; ++k) {
    const double mid = 0.5 * (lo + hi);
    mv.duration = mid;
    if (within_motors(move_torques(m, mv), use)) {
      hi = mid;
    } else {
      lo = mid;
    }
  }
  return hi;
}

// The steady rise of the winding of a joint when the move is repeated with a wait between, holding the end pose: from the rms heat over the cycle.
inline double duty_rise(const ArmModel& m, const Quintic& mv, const JointMotor& motor, std::size_t joint, double wait) {
  const double g[3] = {0.0, 0.0, -9.81};
  const int samples = 200;
  double e = 0.0;
  std::vector<double> q, qd, qdd;
  for (int k = 0; k < samples; ++k) {
    quintic_at(mv, mv.duration * (k + 0.5) / samples, q, qd, qdd);
    const double tau = inverse_dynamics(m, q, qd, qdd, g)[joint];
    e += heat_watts(motor, tau) * mv.duration / samples;
  }
  std::vector<double> qe, zero(mv.q0.size(), 0.0);
  quintic_at(mv, mv.duration, qe, qd, qdd);
  const double hold = inverse_dynamics(m, qe, zero, zero, g)[joint];
  e += heat_watts(motor, hold) * wait;
  return e / (mv.duration + wait) * motor.r_th;
}


#endif
