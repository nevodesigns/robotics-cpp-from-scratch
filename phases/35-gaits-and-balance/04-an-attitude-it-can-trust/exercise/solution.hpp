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
  (void)gx;  // TODO: add the gyro's rate times dt to the roll and the pitch; if |a| is within gate times g of g, pull the roll toward atan2(ay, az) and the pitch toward atan2(-ax, sqrt(ay2 + az2)) by k_acc dt times the difference; if feet_valid, pull both toward the feet's angles by k_feet dt times the difference
  (void)gy;
  (void)acc;
  (void)feet_roll;
  (void)feet_pitch;
  (void)feet_valid;
  (void)dt;
}

// The roll and pitch that an accelerometer feeling only gravity would give.
inline void accel_angles(const double acc[3], double* roll, double* pitch) {
  *roll = std::atan2(acc[1], acc[2]);
  *pitch = std::atan2(-acc[0], std::sqrt(acc[1] * acc[1] + acc[2] * acc[2]));
}


#endif
