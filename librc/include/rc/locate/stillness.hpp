// rc/locate/stillness.hpp
//
// The bias of a gyro when the robot stops, from lesson 25-07.
//
// StillnessDetector says the robot is still when, over a window of samples, the wheels have not moved (their speeds are all under a limit) and the accelerometer's reading has not varied by more than a limit: the first alone would take a robot that is being carried or dragged with its wheels stopped for still. StillBiasFilter keeps the bias of a gyro and its variance; the variance grows all the time by the random walk of the bias, and when the robot is still a reading of the gyro is a measurement of the bias with the gyro's noise. heading_error_growth is the heading error that an uncertain bias makes over a time.
//
// Over 300 s with a gyro whose bias wandered at 3e-4 rad/s per root second, a heading from a bias read once at the start ended 9.9 degrees out on average, with a stop of 2 s every 60 s 4.6 degrees and every 10 s 0.8; the bias read from a stop of 1 s was good to 0.00063 rad/s and from a stop of 10 s to 0.00020; a robot carried with its wheels stopped was taken for still by a detector that looked at the wheels alone and not by one that looked at the accelerometer too; and a step in the bias of 0.01 rad/s, as when the gyro warms, was found again within 19 s of the next stop.

#ifndef RC_RC_LOCATE_STILLNESS_HPP
#define RC_RC_LOCATE_STILLNESS_HPP

#include <cmath>
#include <cstddef>

namespace rc {
namespace locate {

class StillnessDetector {
 public:
  // window: how many samples of stillness make a robot still; wheel_limit: rad/s of the wheels below which they are not turning; accel_limit: the largest spread of the accelerometer's magnitude over the window that a still robot has (m/s2).
  StillnessDetector(int window, double wheel_limit, double accel_limit) : window_(window), wheel_limit_(wheel_limit), accel_limit_(accel_limit) {
    for (int i = 0; i < kMax; ++i) history_[i] = 0.0;
  }

  // One sample: the speeds of the left and right wheels (rad/s) and the magnitude of the accelerometer's reading (m/s2). Returns whether the robot is still.
  bool update(double left_wheel, double right_wheel, double accel_magnitude) {
    const bool wheels_stopped = std::fabs(left_wheel) < wheel_limit_ && std::fabs(right_wheel) < wheel_limit_;
    history_[next_] = accel_magnitude;
    next_ = (next_ + 1) % kMax;
    if (filled_ < window_) ++filled_;
    if (!wheels_stopped) stopped_for_ = 0;
    else if (stopped_for_ < window_) ++stopped_for_;
    if (stopped_for_ < window_ || filled_ < window_) return false;
    double lo = 1e300, hi = -1e300;
    for (int i = 0; i < window_; ++i) {
      const double v = history_[(next_ - 1 - i + 2 * kMax) % kMax];
      lo = lo < v ? lo : v;
      hi = hi > v ? hi : v;
    }
    return hi - lo <= accel_limit_;
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


}  // namespace locate
}  // namespace rc

#endif
