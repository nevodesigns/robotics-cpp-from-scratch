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
  (void)g;  // TODO: for each of n steps of height from h0 to h1 (the middle of each), the knee torque from stance_torques with four feet down; keep the greatest; add the step's time (seconds / n) over time_to_limit(m, torque) to the life used; torque_ok if the greatest is within m.tau_peak, heat_ok if the life used is under 1
  (void)body_mass;
  (void)m;
  (void)h0;
  (void)h1;
  (void)seconds;
  (void)n;
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
