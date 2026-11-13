#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cmath>

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


#endif
