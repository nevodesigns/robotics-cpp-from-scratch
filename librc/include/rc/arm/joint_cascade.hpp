// rc/arm/joint_cascade.hpp
//
// A gain that knows the pose, from lesson 29-03.
//
// effective_inertia is what one joint of the arm sees: the diagonal term of the mass matrix for the pose, plus the rotor's inertia through the gear ratio squared. scheduled_gains gives, for a wanted bandwidth of the position loop and of the velocity loop, the gains of the cascade for that inertia: the velocity loop's proportional gain is the inertia times its bandwidth, its integral gain puts its zero at a quarter of that bandwidth, and the position loop's gain is its bandwidth. step_response runs a joint of a given inertia through the cascade with a step of position: the position loop asks for a velocity (limited), the velocity loop for a torque (limited, the integral held at the limit if asked), and the current loop is a lag; a delay of a few samples sits in the velocity measurement and the command, as in a real drive. It gives the overshoot, the time to settle within two percent, the largest torque, and whether the run blew up.
//
// Over the poses of the typical arm the shoulder's inertia went from 0.691 to 2.091 kg m2, a ratio of 3.0. With the gains for the heaviest pose a step at the heaviest overshot 0.0 percent and the same gains at the lightest pose 13909338; with gains that followed the inertia, 0.0 percent at the lightest and 0.0 at the heaviest. Through a gear of 100:1 with a rotor of 1e-4 kg m2 the ratio fell to 1.8. A step of 1.5 rad into a limit of 20 N m overshot 186 percent with the integral running on and 19.0 with it held.

#ifndef RC_RC_ARM_JOINT_CASCADE_HPP
#define RC_RC_ARM_JOINT_CASCADE_HPP

#include <cmath>
#include <cstddef>
#include <rc/arm/dynamics3d.hpp>
#include <vector>

namespace rc {
namespace arm {

struct CascadeGains {
  double kp_pos = 0.0;  // 1/s: the velocity asked for per radian of error
  double kp_vel = 0.0;  // N m s/rad
  double ki_vel = 0.0;  // N m/rad
};

// What one joint sees: the diagonal of the mass matrix plus the rotor through the gear ratio squared.
inline double effective_inertia(const ArmModel& m, const std::vector<double>& q, std::size_t joint, double rotor_inertia, double gear_ratio) {
  return mass_matrix(m, q)[joint][joint] + gear_ratio * gear_ratio * rotor_inertia;
}

// The gains for a wanted bandwidth of the position loop (wp, rad/s) and of the velocity loop (wv, rad/s) at an inertia.
inline CascadeGains scheduled_gains(double inertia, double wp, double wv) {
  CascadeGains g;
  g.kp_pos = wp;
  g.kp_vel = 1.0;
  g.ki_vel = 0.0;
  g.kp_vel = inertia * wv;
  g.ki_vel = g.kp_vel * wv / 4.0;
  return g;
}

struct StepResult {
  bool stable = true;
  double overshoot_percent = 0.0;
  double settle_time = 0.0;  // s to stay within 2 percent, or the length of the run if it never does
  double max_torque = 0.0;
};

struct LoopConfig {
  double dt = 0.001;
  double current_tc = 0.0015;  // s, the lag of the current loop
  int delay = 2;               // samples of delay in the velocity measurement and the command
  double viscous = 0.01;       // N m s/rad
  double tau_max = 1e9;        // N m
  double v_max = 3.0;          // rad/s asked of the velocity loop at most
  bool anti_windup = true;     // hold the integral while the torque is at its limit
  double seconds = 2.0;
};

// A step of position of `size` radians through the cascade, for a joint of a given inertia.
inline StepResult step_response(double inertia, const CascadeGains& g, double size, const LoopConfig& c) {
  StepResult out;
  const int steps = static_cast<int>(c.seconds / c.dt);
  double q = 0.0, qd = 0.0, torque = 0.0, integ = 0.0;
  std::vector<double> qd_hist(static_cast<std::size_t>(c.delay) + 1, 0.0), cmd_hist(static_cast<std::size_t>(c.delay) + 1, 0.0);
  double peak = 0.0, last_outside = 0.0;
  for (int k = 0; k < steps; ++k) {
    const double e = size - q;
    double v_cmd = g.kp_pos * e;
    v_cmd = std::fmax(-c.v_max, std::fmin(c.v_max, v_cmd));
    const double ev = v_cmd - qd_hist.front();
    double tau_cmd = g.kp_vel * ev + integ;
    const double sat = std::fmax(-c.tau_max, std::fmin(c.tau_max, tau_cmd));
    if (!c.anti_windup || sat == tau_cmd) integ += g.ki_vel * ev * c.dt;
    tau_cmd = sat;
    out.max_torque = std::fmax(out.max_torque, std::fabs(tau_cmd));
    cmd_hist.push_back(tau_cmd);
    const double applied = cmd_hist.front();
    cmd_hist.erase(cmd_hist.begin());
    torque += (applied - torque) * c.dt / c.current_tc;
    qd += (torque - c.viscous * qd) / inertia * c.dt;
    q += qd * c.dt;
    qd_hist.push_back(qd);
    qd_hist.erase(qd_hist.begin());
    if (!(std::fabs(q) < 1e6)) {
      out.stable = false;
      out.overshoot_percent = 1e6;
      out.settle_time = c.seconds;
      return out;
    }
    peak = std::fmax(peak, q);
    if (std::fabs(q - size) > 0.02 * std::fabs(size)) last_outside = (k + 1) * c.dt;
  }
  out.overshoot_percent = size != 0.0 ? 100.0 * std::fmax(0.0, (peak - size) / size) : 0.0;
  out.settle_time = last_outside;
  // A run that is still swinging at the end, with a growing amplitude, is not stable even if it has not blown up yet.
  if (std::fabs(q - size) > 0.5 * std::fabs(size)) out.stable = false;
  return out;
}


}  // namespace arm
}  // namespace rc

#endif
