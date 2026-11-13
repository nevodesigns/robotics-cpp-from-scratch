#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cmath>

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


#endif
