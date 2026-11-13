// rc/legged/push_recovery.hpp
//
// Taking a step when pushed, from lesson 35-07.
//
// capture_point gives the capture point of a body that is a distance x from the foot (or from the line of the feet) that is down and moving away from it at v, a height h up: x plus v times the square root of h over g, which is the place a new foot could be put for the body to come to rest over it. While nothing is done it moves away exponentially: capture_at gives it a time t later, xi0 times e to the t over T. step_target is where the swinging foot must land if it will land after a time t_land (the capture point then, plus a margin) and whether that is within the reach of the leg from the support; max_push_speed is the largest speed a push can give the body, from rest over the support, for which the step can still be taken in time.
//
// A body 0.27 m up has a time constant of 0.166 s. A push of 0.30 m/s sideways, from rest over the line of the stance feet, put its capture point 5.0 cm out; a foot with a swing of 0.25 s had to land 24.5 cm out, 1 (1 means within the 25 cm reach). The largest push that could be answered with swings of 0.15, 0.25 and 0.35 s was 0.56, 0.31 and 0.17 m/s. The foot landed at that place left the capture point 2.0 cm inside it.

#ifndef RC_RC_LEGGED_PUSH_RECOVERY_HPP
#define RC_RC_LEGGED_PUSH_RECOVERY_HPP

#include <cmath>

namespace rc {
namespace legged {

struct StepPlan {
  bool ok = false;        // the place is within the leg's reach
  double target = 0.0;    // m from the support, where the foot must land
  double xi_land = 0.0;   // m, the capture point at the landing time
};

inline double time_constant(double h) { return std::sqrt(h / 9.81); }

// The capture point of a body x from the support, moving away from it at v, a height h up.
inline double capture_point(double x, double v, double h) { return x + v * time_constant(h); }

// The capture point a time t later, if nothing is done.
inline double capture_at(double xi0, double t, double h) { return xi0 * std::exp(t / time_constant(h)); }

// Where a foot landing after t_land must land (the capture point then plus a margin), and whether it is within `reach` of the support.
inline StepPlan step_target(double xi0, double t_land, double h, double reach, double margin) {
  StepPlan p;
  p.xi_land = capture_at(xi0, t_land, h);
  p.target = p.xi_land + margin;
  p.ok = p.target <= reach;
  return p;
}

// The largest speed a push can give a body at rest over the support (capture point v T) for which the step can still be taken in time.
inline double max_push_speed(double reach, double margin, double t_land, double h) {
  const double T = time_constant(h);
  const double xi0 = (reach - margin) * std::exp(-t_land / T);
  return xi0 > 0.0 ? xi0 / T : 0.0;
}


}  // namespace legged
}  // namespace rc

#endif
