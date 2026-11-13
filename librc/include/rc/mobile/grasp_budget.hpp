// rc/mobile/grasp_budget.hpp
//
// Knowing where well enough to grasp, from lesson 33-07.
//
// Sigmas holds the standard deviations of the independent errors: the base's position along an axis (metres), its heading (radians), the arm's mounting turn (radians) and the arm's own repeatability (metres). hand_sigma combines them into one standard deviation for the hand along an axis: the square root of the sum of the squares, with the two angles added in quadrature and multiplied by the lever, the distance from the base's centre to the hand. grasp_probability is the chance that the error is within a tolerance, which for a normal error is the error function of the tolerance over sigma times the square root of two. z_for_probability is its inverse, found by halving an interval: how many sigmas a wanted chance needs. sigma_needed is the tolerance over that. sigma_after_servo is the budget once a camera on the hand has measured the part relative to the hand: the base's errors drop out and the camera's error at that range and the arm's repeatability are what remain.
//
// With a base known to 20 mm and 1 degree, a mount known to 0.2 degree and an arm repeatable to 0.1 mm, the hand was uncertain by 25.6 mm at a reach of 0.9 m; a tolerance of 10 mm gave a chance of 0.30 that the grasp worked, which 20000 simulated grasps gave as 0.30. 1.96 sigmas make 95 percent and 2.58 make 99. For 99 percent the hand's sigma had to be 3.9 mm. With a camera on the hand good to 2 mm the sigma fell to 2.0 mm and the chance to 1.000.

#ifndef RC_RC_MOBILE_GRASP_BUDGET_HPP
#define RC_RC_MOBILE_GRASP_BUDGET_HPP

#include <cmath>

namespace rc {
namespace mobile {

struct Sigmas {
  double pos = 0.020;        // m, the base's place along an axis (typical of a lidar localisation)
  double heading = 0.01745;  // rad, the base's heading (one degree)
  double mount = 0.0035;     // rad, the arm's mounting turn (0.2 degree)
  double arm = 0.0001;       // m, the arm's repeatability
};

// The standard deviation of the hand's error along an axis, at `lever` metres from the base's centre.
inline double hand_sigma(const Sigmas& s, double lever) {
  const double lateral = lever * std::sqrt(s.heading * s.heading + s.mount * s.mount);
  return std::sqrt(s.pos * s.pos + lateral * lateral + s.arm * s.arm);
}

// The chance that a normal error of standard deviation sigma is within plus or minus tol.
inline double grasp_probability(double sigma, double tol) {
  if (sigma <= 0.0) return 1.0;
  return std::erf(tol / (sigma * 1.4142135623730951));
}

// How many standard deviations a chance p (0 to 1) of being within plus or minus needs.
inline double z_for_probability(double p) {
  double lo = 0.0, hi = 10.0;
  for (int i = 0; i < 80; ++i) {
    const double mid = 0.5 * (lo + hi);
    if (std::erf(mid / 1.4142135623730951) < p) {
      lo = mid;
    } else {
      hi = mid;
    }
  }
  return 0.5 * (lo + hi);
}

// The hand's sigma that a tolerance needs for a chance p.
inline double sigma_needed(double tol, double p) { return tol / z_for_probability(p); }

// The budget once a camera on the hand has measured the part relative to the hand: the base's errors drop out.
inline double sigma_after_servo(double camera_sigma, double arm_sigma) { return std::sqrt(camera_sigma * camera_sigma + arm_sigma * arm_sigma); }


}  // namespace mobile
}  // namespace rc

#endif
