// rc/arm/ssm.hpp
//
// Staying far enough from a person, from lesson 31-06. This is a model of an idea in ISO/TS 15066 and ISO 13855 and is not a safety function: a real one is designed and validated against the current edition of those standards.
//
// SsmParams holds the human's approach speed v_h, the system's reaction time t_r (sensing and processing until braking starts), the robot's braking deceleration, the intrusion distance c (how far the body can reach in before it is sensed) and the two measurement uncertainties z_d (of the person's place) and z_r (of the robot's). protective_distance gives the separation at which the robot, moving toward the person at speed v_r, must already be stopping: v_h (t_r + t_s) + v_r t_r + v_r squared over twice the deceleration + c + z_d + z_r, with the stopping time t_s = v_r over the deceleration. max_speed is the inverse: the greatest v_r whose protective distance is at most a given separation, zero if even v_r = 0 does not fit.
//
// With a person at 1.6 m/s, a reaction time of 100 ms, a braking deceleration of 5 m/s2 and 17 cm of intrusion and uncertainty, a robot at 1.0 m/s needed 0.85 m of separation, and at rest 0.33 m. At 1.0 m the robot could go at 1.23 m/s and at 2.0 m at 2.49 m/s. In a simulation of a person walking at the robot, the one that stopped at the right distance came to rest 0.17 m from the person; one that left out the person's motion, -0.31 m (negative: they met); one that left out the reaction time, -0.09 m (also negative).

#ifndef RC_RC_ARM_SSM_HPP
#define RC_RC_ARM_SSM_HPP

#include <cmath>

namespace rc {
namespace arm {

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
  const double k = p.c + p.z_d + p.z_r + p.v_h * p.t_r - separation;
  const double a = 0.5 / p.decel;
  const double b = p.v_h / p.decel + p.t_r;
  if (k >= 0.0) return 0.0;
  const double disc = b * b - 4.0 * a * k;
  return (-b + std::sqrt(disc)) / (2.0 * a);
}


}  // namespace arm
}  // namespace rc

#endif
