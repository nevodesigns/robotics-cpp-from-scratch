// rc/legged/get_up.hpp
//
// Getting up, from lesson 36-03.
//
// rise_plan takes a leg's geometry, the body's mass, the motor, the height of the body when it is crouched, the height standing and the time the rise takes, and goes through the rise in small steps of height (the knee torque at each, from the stance torque of 34-06 with four feet down): it keeps the greatest torque, whether that is under the motor's peak, and the share of the knee's thermal life that the rise uses (the sum over the steps of the time spent over the time that torque would take to bring the winding to its limit, from 29-08; one means it reaches the limit just as the rise ends). max_rising_mass is the heaviest body that can complete the rise within the peak torque and within the heat, found by halving an interval.
//
// A 12 kg robot rising from a crouch of 0.12 m to 0.28 m needed a knee torque of up to 5.9 N m at the bottom, against a peak of 20 N m; the rise used 0.034 of the knee's thermal life in 1 s and 0.202 in 6 s. The heaviest body that could rise was 40.6 kg by torque, 40.6 kg by heat in 1 s and 24.8 kg by heat in 6 s. A 65 kg robot 0 (1 means it could).

#ifndef RC_RC_LEGGED_GET_UP_HPP
#define RC_RC_LEGGED_GET_UP_HPP

#include <cmath>
#include <rc/arm/limits_heat.hpp>
#include <rc/legged/leg_kin.hpp>
#include <rc/legged/leg_load.hpp>

namespace rc {
namespace legged {

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


}  // namespace legged
}  // namespace rc

#endif
