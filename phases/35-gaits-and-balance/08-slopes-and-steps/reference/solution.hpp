#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cmath>
#include <rc/legged/foot_path.hpp>
#include <rc/legged/leg_kin.hpp>

using namespace rc::legged;

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


#endif
