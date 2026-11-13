#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cmath>
#include <cstddef>
#include <rc/arm/dynamics3d.hpp>
#include <rc/arm/ik_dls.hpp>
#include <rc/arm/jacobian.hpp>
#include <vector>

using namespace rc::arm;

struct Surface {
  double z = 0.0;       // the height of the plane
  double k = 20000.0;   // N/m
  double c = 20.0;      // N s/m
};

// The upward force of the surface on a tool at a height and a vertical speed (nothing if the tool is above it).
inline double contact_force(const Surface& s, double z, double vz) {
  if (z >= s.z) return 0.0;
  const double f = s.k * (s.z - z) - s.c * vz;
  return f > 0.0 ? f : 0.0;
}

// The joint torques that make the tool a spring and damper about a target pose: the wrench through J^T, plus the torque of gravity of the model.
inline std::vector<double> impedance_torque(const ArmModel& m, const std::vector<double>& q, const std::vector<double>& qd, const Pose& target, const Pose& tool, const double stiffness[6], const double damping[6]) {
  const Pose now = forward(m.table, q, Pose(), tool);
  double e[6];
  pose_error(target, now, e);
  const Jacobian j = jacobian(m.table, q, Pose(), tool, m.table.convention);
  double v[6];
  tool_twist(j, qd, v);
  double wrench[6];
  for (int r = 0; r < 6; ++r) wrench[r] = stiffness[r] * e[r] - damping[r] * v[r];
  std::vector<double> tau = joint_torques(j, wrench);
  const std::vector<double> tg = gravity_torques(m, q);
  for (std::size_t i = 0; i < tau.size(); ++i) tau[i] += tg[i];
  return tau;
}

struct PressResult {
  double peak_force = 0.0;
  double settled_force = 0.0;
  bool steady = false;
};

// The arm under an impedance controller from rest at q0, with the target `target`, touching a surface; for some seconds.
inline PressResult press(const ArmModel& m, const std::vector<double>& q0, const Pose& target, const Pose& tool, double kz, double dz, const Surface& surf, double seconds, double dt) {
  PressResult out;
  std::vector<double> q = q0, qd(q0.size(), 0.0);
  double stiff[6] = {2000.0, 2000.0, kz, 10.0, 10.0, 10.0}, damp[6] = {60.0, 60.0, dz, 0.5, 0.5, 0.5};
  const int steps = static_cast<int>(seconds / dt);
  double f_last = 0.0, lo = 1e9, hi = -1e9;
  for (int k = 0; k < steps; ++k) {
    const Pose now = forward(m.table, q, Pose(), tool);
    const Jacobian j = jacobian(m.table, q, Pose(), tool, m.table.convention);
    double v[6];
    tool_twist(j, qd, v);
    const double f = contact_force(surf, now.p[2], v[2]);
    double ext[6] = {0.0, 0.0, f, 0.0, 0.0, 0.0};
    std::vector<double> tau = impedance_torque(m, q, qd, target, tool, stiff, damp);
    const std::vector<double> te = joint_torques(j, ext);
    for (std::size_t i = 0; i < tau.size(); ++i) tau[i] += te[i];
    step_dynamics(m, q, qd, tau, dt, true);
    if (!(q[0] == q[0]) || !(std::fabs(q[1]) < 1e3)) {
      out.peak_force = 1e9;  // the run blew up
      return out;
    }
    out.peak_force = std::fmax(out.peak_force, f);
    f_last = f;
    if (k * dt > seconds - 0.5) {
      lo = std::fmin(lo, f);
      hi = std::fmax(hi, f);
    }
    if (!(out.peak_force < 1e6)) {
      out.peak_force = 1e9;
      return out;
    }
  }
  out.settled_force = f_last;
  out.steady = (hi - lo) < 0.05 * (std::fabs(f_last) + 1.0);
  return out;
}

struct AdmittanceConfig {
  double mv = 2.0;        // kg, the virtual mass
  double dv = 100.0;      // N s/m, the virtual damper
  double kv = 500.0;      // N/m, the virtual spring about the reference
  double tool_mass = 5.0; // kg, what the inner loop moves
  double inner_kp = 10000.0;
  double delay = 0.005;   // s, the delay in the force measurement
  double dt = 0.001;
};

// A tool on a vertical axis, position controlled by an inner loop, approaching a surface with its reference ramping down to 2 cm below it, with the admittance turning the measured force into a shift of that reference.
inline PressResult admittance_press(const AdmittanceConfig& a, const Surface& surf, double seconds) {
  PressResult out;
  const int steps = static_cast<int>(seconds / a.dt);
  const int d = static_cast<int>(a.delay / a.dt);
  std::vector<double> f_hist(static_cast<std::size_t>(d) + 1, 0.0);
  double x = surf.z + 0.05, v = 0.0;      // the tool
  double shift = 0.0, shift_v = 0.0;      // the admittance's displacement of the reference
  const double inner_kd = 2.0 * std::sqrt(a.inner_kp * a.tool_mass);
  double f_last = 0.0, lo = 1e9, hi = -1e9;
  for (int k = 0; k < steps; ++k) {
    const double t = k * a.dt;
    const double ref = std::fmax(surf.z - 0.02, surf.z + 0.05 - 0.05 * t);
    const double f_meas = f_hist.front();
    // The virtual mass, damper and spring: mv shift'' + dv shift' + kv shift = f_meas (a force upward moves the reference upward).
    const double acc = (f_meas - a.dv * shift_v - a.kv * shift) / a.mv;
    shift_v += acc * a.dt;
    shift += shift_v * a.dt;
    const double x_c = ref + shift;
    const double f_ctrl = a.inner_kp * (x_c - x) - inner_kd * v;
    const double f_env = contact_force(surf, x, v);
    v += (f_ctrl + f_env) / a.tool_mass * a.dt;
    x += v * a.dt;
    f_hist.push_back(f_env);
    f_hist.erase(f_hist.begin());
    out.peak_force = std::fmax(out.peak_force, f_env);
    f_last = f_env;
    if (t > seconds - 0.5) {
      lo = std::fmin(lo, f_env);
      hi = std::fmax(hi, f_env);
    }
    if (!(out.peak_force < 1e6)) {
      out.peak_force = 1e9;
      return out;
    }
  }
  out.settled_force = f_last;
  out.steady = (hi - lo) < 0.05 * (std::fabs(f_last) + 1.0);
  return out;
}


#endif
