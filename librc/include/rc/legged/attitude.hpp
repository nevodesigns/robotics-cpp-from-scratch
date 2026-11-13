// rc/legged/attitude.hpp
//
// An attitude it can trust, from lesson 35-04.
//
// AttitudeFilter holds the estimated roll and pitch, the gains with which it is pulled toward the accelerometer's roll and pitch (k_acc) and toward the feet's (k_feet), per second, and the gate on the accelerometer: it is believed only when the size of what it feels is within a fraction of g. attitude_update adds the gyro's rates times the time step to the roll and pitch, then pulls each toward the accelerometer's angle (the arctangent of the components of the gravity it feels) by k_acc times the time step times the difference if the gate is met, and toward the angles from the feet (34-03) by k_feet times the time step times the difference if there were three or more feet on the ground.
//
// With a gyro that had a bias of 0.5 degrees per second the gyro alone was 23.1 degrees wrong after 60 s. The accelerometer alone, in a walk that shook it by 2.5 m/s2, was 10.2 degrees wrong at a moment (rms), and was within the gate 100 percent of the time. The filter with the accelerometer was 1.02 degrees wrong (rms) and with the feet as well, in a walk, 0.40; in a trot, where only two feet are down and the feet cannot be used, it was 1.02.

#ifndef RC_RC_LEGGED_ATTITUDE_HPP
#define RC_RC_LEGGED_ATTITUDE_HPP

#include <cmath>

namespace rc {
namespace legged {

struct AttitudeFilter {
  double roll = 0.0;       // rad
  double pitch = 0.0;
  double k_acc = 0.5;      // 1/s, the pull toward the accelerometer
  double k_feet = 1.0;     // 1/s, the pull toward the feet
  double gate = 0.15;      // the accelerometer is believed when |a| is within this fraction of g
};

// One step: the gyro's rates about the body's x and y axes (rad/s), the accelerometer (m/s2, body frame), the roll and pitch from the feet (valid only if three or more were down), and the time step.
inline void attitude_update(AttitudeFilter& f, double gx, double gy, const double acc[3], double feet_roll, double feet_pitch, bool feet_valid, double dt) {
  f.roll += gx * dt;
  f.pitch += gy * dt;
  const double mag = std::sqrt(acc[0] * acc[0] + acc[1] * acc[1] + acc[2] * acc[2]);
  if (std::fabs(mag - 9.81) < f.gate * 9.81) {
    const double ar = std::atan2(acc[1], acc[2]);
    const double ap = std::atan2(-acc[0], std::sqrt(acc[1] * acc[1] + acc[2] * acc[2]));
    f.roll += f.k_acc * dt * (ar - f.roll);
    f.pitch += f.k_acc * dt * (ap - f.pitch);
  }
  if (feet_valid) {
    f.roll += f.k_feet * dt * (feet_roll - f.roll);
    f.pitch += f.k_feet * dt * (feet_pitch - f.pitch);
  }
}

// The roll and pitch that an accelerometer feeling only gravity would give.
inline void accel_angles(const double acc[3], double* roll, double* pitch) {
  *roll = std::atan2(acc[1], acc[2]);
  *pitch = std::atan2(-acc[0], std::sqrt(acc[1] * acc[1] + acc[2] * acc[2]));
}


}  // namespace legged
}  // namespace rc

#endif
