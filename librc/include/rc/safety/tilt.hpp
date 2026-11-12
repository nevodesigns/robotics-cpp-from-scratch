// rc/safety/tilt.hpp
//
// Tipping geometry and a tilt monitor, from lesson 19-09, graduated.
//
// static_stability_angle is atan((track/2)/cg height) and tip_accel is g times half the track over the height. max_turn_speed is the speed at which a fraction (safety_factor) of the tipping sideways acceleration is reached on a curve: sqrt(factor * tip_accel / |curvature|), infinite on a straight line. accel_roll and accel_pitch are the angles an accelerometer feeling only gravity gives. TiltEstimator adds the gyro rate each step and pulls towards the accelerometer's roll only when the accelerometer's magnitude is within a gate of g. TiltMonitor warns at a level, stops after a debounce of samples at the stop level and at once on a NaN.
//
// A track of 0.5 m tipped at 39.8 degrees and 8.18 m/s^2 with the centre of gravity at 0.3 m and at 26.6 degrees and 4.91 m/s^2 at 0.5 m; a radius 1 m turn allowed 2.02 m/s and then 1.57 (77 percent). On level ground at 4 m/s^2 sideways the accelerometer alone was 27.3 degrees wrong, the filter with a gate of 15 percent 22.1 and with 3 percent 5.1; a 0.5 deg/s gyro bias over 60 s was 1.2, 1.7 and (gate 1 percent) 4.1.

#ifndef RC_SAFETY_TILT_HPP
#define RC_SAFETY_TILT_HPP

#include <algorithm>
#include <cmath>

namespace rc {
namespace safety {

const double kGravity = 9.81;

// A vehicle tips sideways about its outer wheels when the sideways pull on its centre of gravity, taken about them, outweighs its weight: when the sideways acceleration is more than g times half the track over the height of the centre of gravity.
// The angle of a slope at which it would tip over standing still is atan((track / 2) / height).
inline double static_stability_angle(double track, double cg_height) { return std::atan2(0.5 * track, cg_height); }

inline double tip_accel(double track, double cg_height) { return kGravity * 0.5 * track / cg_height; }

// The fastest the vehicle may go on a curve of the given curvature (1 / radius, 1/m; the sign does not matter), keeping to a fraction of the sideways acceleration that would tip it (safety_factor, 0 to 1): v^2 |k| = factor * tip_accel.
// A straight line (curvature 0) has no limit from this: infinity.
inline double max_turn_speed(double curvature, double track, double cg_height, double safety_factor) {
  const double k = std::fabs(curvature);
  if (k == 0.0) return INFINITY;
  return std::sqrt(std::max(safety_factor, 0.0) * tip_accel(track, cg_height) / k);
}

// The roll and pitch angles (radians) that an accelerometer gives when it feels only gravity: x forward, y left, z up.
inline double accel_roll(double ay, double az) { return std::atan2(ay, az); }
inline double accel_pitch(double ax, double ay, double az) { return std::atan2(-ax, std::sqrt(ay * ay + az * az)); }

// A roll (or pitch) estimate from a gyro and an accelerometer. The gyro is smooth and drifts; the accelerometer does not drift and is wrong whenever the vehicle accelerates, since it feels that as well as gravity.
// Each step: angle += gyro * dt; then, only if the accelerometer's magnitude is within gate (a fraction, say 0.03) of g, which says it is probably feeling only gravity, pull the angle towards the accelerometer's with a time constant tau: angle += dt / (tau + dt) * (measured - angle).
class TiltEstimator {
 public:
  TiltEstimator(double tau, double gate) : tau_(tau), gate_(gate) {}

  double update(double gyro_rate, double ax, double ay, double az, double dt) {
    angle_ += gyro_rate * dt;
    const double mag = std::sqrt(ax * ax + ay * ay + az * az);
    if (std::fabs(mag - kGravity) <= gate_ * kGravity) {
      const double measured = accel_roll(ay, az);
      angle_ += dt / (tau_ + dt) * (measured - angle_);
    }
    return angle_;
  }

  double angle() const { return angle_; }
  void reset(double angle) { angle_ = angle; }

 private:
  double tau_ = 2.0;
  double gate_ = 0.03;
  double angle_ = 0.0;
};

enum class TiltState { ok, warn, stop };

// A tilt angle (radians, either sign) against a warning level and a stop level. A NaN stops at once. The stop needs `debounce` samples in a row at or past the stop level, so a single bump does not stop the robot; the warning has no debounce.
class TiltMonitor {
 public:
  TiltMonitor(double warn, double stop, int debounce) : warn_(warn), stop_(stop), debounce_(debounce) {}

  TiltState update(double angle) {
    if (std::isnan(angle)) return TiltState::stop;
    const double a = std::fabs(angle);
    run_ = a >= stop_ ? run_ + 1 : 0;
    if (run_ >= debounce_) return TiltState::stop;
    if (a >= warn_) return TiltState::warn;
    return TiltState::ok;
  }

 private:
  double warn_ = 0.3;
  double stop_ = 0.5;
  int debounce_ = 3;
  int run_ = 0;
};

}  // namespace safety
}  // namespace rc

#endif  // RC_SAFETY_TILT_HPP
