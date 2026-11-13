#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cmath>
#include <rc/arm/limits_heat.hpp>
#include <rc/legged/leg_kin.hpp>
#include <rc/legged/leg_load.hpp>

using namespace rc::legged;

using rc::arm::JointMotor;
using rc::arm::time_to_limit;

struct RisePlan {
  double tau_peak = 0.0;   // N m, the greatest knee torque in the rise
  bool torque_ok = false;  // under the motor's peak torque
  double life_used = 0.0;  // the share of the knee's thermal life the rise uses
  bool heat_ok = false;    // under 1
};

// The rise from a crouch at h0 to a stand at h1 in `seconds`, four feet down, in n steps.
inline RisePlan rise_plan(const LegGeom& g, double body_mass, const JointMotor& m, double h0, double h1, double seconds, int n) {
  RisePlan p;
  for (int i = 0; i < n; ++i) {
    const double h = h0 + (h1 - h0) * (i + 0.5) / n;
    double tau[3];
    stance_torques(g, h, body_mass, 4, tau);
    const double t = std::fabs(tau[2]);
    if (t > p.tau_peak) p.tau_peak = t;
    const double limit_time = time_to_limit(m, t);
    p.life_used += (seconds / n) / limit_time;
  }
  p.torque_ok = p.tau_peak <= m.tau_peak;
  p.heat_ok = p.life_used < 1.0;
  return p;
}

// The heaviest body that completes the rise within the peak torque (by_heat false) or within the heat as well (true).
inline double max_rising_mass(const LegGeom& g, const JointMotor& m, double h0, double h1, double seconds, bool by_heat) {
  double lo = 0.0, hi = 200.0;
  for (int i = 0; i < 60; ++i) {
    const double mid = 0.5 * (lo + hi);
    const RisePlan p = rise_plan(g, mid, m, h0, h1, seconds, 40);
    const bool ok = p.torque_ok && (!by_heat || p.heat_ok);
    if (ok) {
      lo = mid;
    } else {
      hi = mid;
    }
  }
  return lo;
}


#endif
