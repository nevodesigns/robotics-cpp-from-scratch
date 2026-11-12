#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cmath>
#include <cstddef>

class StillnessDetector {
 public:
  // window: how many samples of stillness make a robot still; wheel_limit: rad/s of the wheels below which they are not turning; accel_limit: the largest spread of the accelerometer's magnitude over the window that a still robot has (m/s2).
  StillnessDetector(int window, double wheel_limit, double accel_limit) : window_(window), wheel_limit_(wheel_limit), accel_limit_(accel_limit) {
    for (int i = 0; i < kMax; ++i) history_[i] = 0.0;
  }

  // One sample: the speeds of the left and right wheels (rad/s) and the magnitude of the accelerometer's reading (m/s2). Returns whether the robot is still.
  bool update(double left_wheel, double right_wheel, double accel_magnitude) {
    (void)left_wheel;  // TODO: remember the accelerometer magnitudes of the last window samples; count how many samples in a row both wheels have been under the limit; the robot is still if that count has reached the window and the spread (largest less smallest) of the magnitudes over the window is within the limit
    (void)right_wheel;
    (void)accel_magnitude;
    return false;
  }

 private:
  static const int kMax = 512;
  int window_;
  double wheel_limit_;
  double accel_limit_;
  double history_[kMax];
  int next_ = 0;
  int filled_ = 0;
  int stopped_for_ = 0;
};

// The bias of a gyro: a random walk, observed only when the robot is still.
class StillBiasFilter {
 public:
  // gyro_noise: standard deviation of a reading (rad/s); walk: how fast the bias wanders (rad/s per root second); start: the first guess and its standard deviation.
  StillBiasFilter(double gyro_noise, double walk, double start, double start_sigma) : r_(gyro_noise * gyro_noise), q_(walk * walk), b_(start), p_(start_sigma * start_sigma) {}

  double bias() const { return b_; }
  double variance() const { return p_; }

  // One step of dt: the bias is less certain; and if the robot is still, the gyro reading is a measurement of it.
  void update(double gyro, bool still, double dt) {
    p_ += q_ * dt;
    if (!still) return;
    const double k = p_ / (p_ + r_);
    b_ += k * (gyro - b_);
    p_ *= (1.0 - k);
  }

 private:
  double r_;
  double q_;
  double b_;
  double p_;
};

// The heading error that a bias known only to a standard deviation makes over a time.
inline double heading_error_growth(double bias_sigma, double seconds) { return bias_sigma * seconds; }


#endif
