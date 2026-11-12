#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <algorithm>
#include <cmath>

const double kGravity = 9.81;

// A vehicle tips sideways about its outer wheels when the sideways pull on its centre of gravity, taken about them, outweighs its weight: when the sideways acceleration is more than g times half the track over the height of the centre of gravity.
// The angle of a slope at which it would tip over standing still is atan((track / 2) / height).
inline double static_stability_angle(double track, double cg_height) {
  // TODO: atan2(track / 2, cg_height)
  (void)track;
  (void)cg_height;
  return 0.0;
}

inline double tip_accel(double track, double cg_height) {
  // TODO: kGravity * (track / 2) / cg_height
  (void)track;
  (void)cg_height;
  return 0.0;
}

// The fastest the vehicle may go on a curve of the given curvature (1 / radius, 1/m; the sign does not matter), keeping to a fraction of the sideways acceleration that would tip it (safety_factor, 0 to 1): v^2 |k| = factor * tip_accel.
// A straight line (curvature 0) has no limit from this: infinity.
inline double max_turn_speed(double curvature, double track, double cg_height, double safety_factor) {
  // TODO: infinity for zero curvature; otherwise sqrt(max(safety_factor, 0) * tip_accel(track, cg_height) / |curvature|)
  (void)curvature;
  (void)track;
  (void)cg_height;
  (void)safety_factor;
  return 0.0;
}

// The roll and pitch angles (radians) that an accelerometer gives when it feels only gravity: x forward, y left, z up.
inline double accel_roll(double ay, double az) {
  // TODO: atan2(ay, az)
  (void)ay;
  (void)az;
  return 0.0;
}
inline double accel_pitch(double ax, double ay, double az) {
  // TODO: atan2(-ax, sqrt(ay*ay + az*az))
  (void)ax;
  (void)ay;
  (void)az;
  return 0.0;
}

// A roll (or pitch) estimate from a gyro and an accelerometer. The gyro is smooth and drifts; the accelerometer does not drift and is wrong whenever the vehicle accelerates, since it feels that as well as gravity.
// Each step: angle += gyro * dt; then, only if the accelerometer's magnitude is within gate (a fraction, say 0.03) of g, which says it is probably feeling only gravity, pull the angle towards the accelerometer's with a time constant tau: angle += dt / (tau + dt) * (measured - angle).
class TiltEstimator {
 public:
  TiltEstimator(double tau, double gate) : tau_(tau), gate_(gate) {}

  double update(double gyro_rate, double ax, double ay, double az, double dt) {
    // TODO: angle_ += gyro_rate * dt. Then if | |a| - kGravity | <= gate_ * kGravity, with |a| the magnitude of (ax, ay, az): angle_ += dt / (tau_ + dt) * (accel_roll(ay, az) - angle_). Return angle_
    (void)gyro_rate;
    (void)ax;
    (void)ay;
    (void)az;
    (void)dt;
    (void)tau_;
    (void)gate_;
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
    // TODO: NaN: stop. run_ counts samples in a row with |angle| >= stop_ (zero otherwise); stop if run_ >= debounce_; warn if |angle| >= warn_; else ok
    (void)angle;
    (void)warn_;
    (void)stop_;
    (void)debounce_;
    (void)run_;
    return TiltState::ok;
  }

 private:
  double warn_ = 0.3;
  double stop_ = 0.5;
  int debounce_ = 3;
  int run_ = 0;
};

#endif  // LESSON_SOLUTION_HPP
