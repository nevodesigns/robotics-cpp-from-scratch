#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cmath>
#include <rc/arm/limits_heat.hpp>
#include <rc/legged/leg_force.hpp>
#include <rc/legged/leg_kin.hpp>

using namespace rc::legged;

using rc::arm::heat_watts;
using rc::arm::JointMotor;
using rc::arm::time_to_limit;

// The torques at the three joints of a leg standing with its foot a height h below the hip (straight below the hip's pitch axis), carrying the body's weight divided among `feet_down` feet.
inline void stance_torques(const LegGeom& g, double h, double body_mass, int feet_down, double tau[3]) {
  double q[3], j[3][3];
  leg_ik(g, 0.0, g.side * g.l0, -h, -1, q);
  leg_jacobian(g, q[0], q[1], q[2], j);
  const double ground[3] = {0.0, 0.0, body_mass * 9.81 / feet_down};
  stand_torques(j, ground, tau);
}

// The root mean square of a joint's torque over a gait cycle: the stance torque for the duty fraction and the swing torque for the rest.
inline double cycle_rms_torque(double tau_stance, double tau_swing, double duty) {
  (void)tau_swing;  // TODO: the square root of (duty times the stance torque squared plus (1 - duty) times the swing torque squared)
  (void)duty;
  return tau_stance;
}

// The temperature the windings settle to above the air with a torque held (or an rms torque over a cycle).
inline double winding_rise(const JointMotor& m, double torque) { return heat_watts(m, torque) * m.r_th; }

// The heaviest body for which the knee's settling temperature stays under its limit, in a gait with `feet_down` feet carrying the weight in the stance, for the duty and the swing torque given.
inline double max_body_mass(const LegGeom& g, const JointMotor& m, double h, int feet_down, double duty, double tau_swing) {
  double lo = 0.0, hi = 200.0;
  for (int i = 0; i < 60; ++i) {
    const double mid = 0.5 * (lo + hi);
    double tau[3];
    stance_torques(g, h, mid, feet_down, tau);
    const double rms = cycle_rms_torque(std::fabs(tau[2]), tau_swing, duty);
    if (winding_rise(m, rms) <= m.rise_limit) {
      lo = mid;
    } else {
      hi = mid;
    }
  }
  return lo;
}


#endif
