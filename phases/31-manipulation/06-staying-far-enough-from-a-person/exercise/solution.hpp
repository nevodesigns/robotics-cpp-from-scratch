#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cmath>

struct SsmParams {
  double v_h = 1.6;     // m/s, the speed a person approaches at (the usual walking speed of ISO 13855; read the current edition)
  double t_r = 0.10;    // s, from the person being in the zone to the robot starting to brake (typical)
  double decel = 5.0;   // m/s2, braking (typical, a stop category 2 within the limits of 29-08)
  double c = 0.10;      // m, how far the body can reach before it is sensed (typical)
  double z_d = 0.05;    // m, the uncertainty in where the person is (typical)
  double z_r = 0.02;    // m, the uncertainty in where the robot is (typical)
};

// The separation at which a robot moving toward a person at v_r must already be braking.
inline double protective_distance(const SsmParams& p, double v_r) {
  const double t_s = v_r / p.decel;
  return p.v_h * (p.t_r + t_s) + v_r * p.t_r + 0.5 * v_r * v_r / p.decel + p.c + p.z_d + p.z_r;
}

// The greatest speed toward the person that a separation allows, 0 if even standing still does not fit.
inline double max_speed(const SsmParams& p, double separation) {
  (void)p;  // TODO: protective_distance is a quadratic in v_r (a = 1/(2 decel), b = v_h/decel + t_r, c = everything else less the separation); take the positive root, 0 if the constant term is not negative
  (void)separation;
  return 0.0;
}


#endif
