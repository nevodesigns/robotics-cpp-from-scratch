// rc/legged/terrain_limits.hpp
//
// Slopes and steps, from lesson 35-08.
//
// max_slope_friction is the arctangent of the friction. max_slope_tip is the arctangent of the half wheelbase over the height of the centre of mass: the slope at which the weight's line falls on the downhill feet when climbing or descending (the formula of 19-09, along the body). max_slope_level is the steepest slope for which the body can stay level, the uphill feet reaching up and the downhill feet down by the half wheelbase times the slope, from the spare length of the legs. max_step_height is the highest step a foot can be put on ahead of the hip at a given distance, found by halving an interval with the inverse kinematics of 34-01, counting a place as reachable only if the knee stays within its limit; max_step_servo is the highest the swing can lift the foot in a given time without a joint needing more rate than the servos have, from the joint rates of 34-05.
//
// For a robot standing 0.27 m up with legs that can stretch 0.10 m up and 0.12 m down, the feet slipped on a slope of 31.0 degrees (a friction of 0.6) and 16.7 degrees when wet (0.3), the body tipped at 35.1 and a body kept level could not reach beyond 27.8: the first to give was 2 (0 friction, 1 tipping, 2 reach). A foot 0.10 m ahead of the hip could be put on a step 0.229 m high by the legs' geometry, and 0.033 m when the swing took 0.20 s and the servos give 8 rad/s.

#ifndef RC_RC_LEGGED_TERRAIN_LIMITS_HPP
#define RC_RC_LEGGED_TERRAIN_LIMITS_HPP

#include <cmath>
#include <rc/legged/foot_path.hpp>
#include <rc/legged/leg_kin.hpp>

namespace rc {
namespace legged {

inline double max_slope_friction(double mu) { return std::atan(mu); }

// The slope at which the weight's line leaves the downhill feet: the arctangent of the half wheelbase over the height of the centre of mass.
inline double max_slope_tip(double half_wheelbase, double com_height) { return std::atan(half_wheelbase / com_height); }

// The steepest slope for which a body kept level can still have every foot on the ground: the uphill feet reach up and the downhill feet down by (half wheelbase times the slope), so the slope is the arctangent of the legs' spare length (the less of the two) over the half wheelbase.
inline double max_slope_level(double half_wheelbase, double spare_up, double spare_down) {
  const double spare = spare_up < spare_down ? spare_up : spare_down;
  return std::atan(spare / half_wheelbase);
}

// The highest a foot can be put, a distance x ahead of the hip (and straight out at the hip's width), when it stands h below the hip: the largest rise s for which the inverse kinematics has a solution with the knee between knee_lo and knee_hi (the knee angle is negative when bent back).
inline double max_step_height(const LegGeom& g, double x, double h, double knee_lo, double knee_hi) {
  double lo = 0.0, hi = h;
  for (int i = 0; i < 60; ++i) {
    const double s = 0.5 * (lo + hi);
    double q[3];
    const bool ok = leg_ik(g, x, g.side * g.l0, -(h - s), -1, q) && q[2] >= knee_lo && q[2] <= knee_hi;
    if (ok) {
      lo = s;
    } else {
      hi = s;
    }
  }
  return lo;
}

// The highest step a swing of t_sw seconds can lift the foot onto without a joint needing more than `rate` rad/s (the clearance is the step plus `clear`); the foot lands x ahead of where it left, at the same distance below the hip plus the rise.
inline double max_step_servo(const LegGeom& g, double x, double h, double t_sw, double clear, double rate) {
  double lo = 0.0, hi = 0.3;
  for (int i = 0; i < 40; ++i) {
    const double s = 0.5 * (lo + hi);
    const double p0[3] = {-0.5 * x, g.side * g.l0, -h}, p1[3] = {0.5 * x, g.side * g.l0, -(h - s)};
    double peak = 0.0;
    if (swing_peak_joint_rate(g, p0, p1, s + clear, t_sw, -1, &peak) && peak <= rate) {
      lo = s;
    } else {
      hi = s;
    }
  }
  return lo;
}


}  // namespace legged
}  // namespace rc

#endif
