#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cmath>

// An estimate of a gyro's bias, kept by watching how the gyro differs from the turn rate the wheels give.
class GyroBiasFilter {
 public:
  // gyro_noise: the standard deviation of one gyro reading (rad/s); wheel_error: the share of its own turn rate that the wheels are wrong by; walk: how fast the bias may wander (rad/s per root second); gate: how many standard deviations a difference may be from the estimate before it is taken for a slip.
  GyroBiasFilter(double gyro_noise, double wheel_error, double walk, double gate) : sigma_(gyro_noise), share_(wheel_error), walk_(walk), gate_(gate) {}

  double bias() const { return b_; }
  double variance() const { return p_; }
  int refused() const { return refused_; }

  // One step: gyro and wheels' turn rates (rad/s) and dt. Returns the gyro's turn rate less the bias.
  double update(double gyro, double wheel_rate, double dt) {
    p_ += walk_ * walk_ * dt;
    (void)wheel_rate;  // TODO: the measurement of the bias is gyro - wheel_rate, with a variance of sigma^2 + (share * wheel_rate)^2; if it is within gate standard deviations of the estimate, a scalar Kalman update; otherwise count it as refused
    return gyro - b_;
  }

 private:
  double sigma_;
  double share_;
  double walk_;
  double gate_;
  double b_ = 0.0;
  double p_ = 1.0e-2;  // a bias of 0.1 rad/s would not surprise it at the start
  int refused_ = 0;
};

// A heading that integrates the gyro less its bias.
class FusedHeading {
 public:
  FusedHeading(double gyro_noise, double wheel_error, double walk, double gate) : bias_(gyro_noise, wheel_error, walk, gate) {}
  void update(double gyro, double wheel_rate, double dt) { theta_ += bias_.update(gyro, wheel_rate, dt) * dt; }
  double theta() const { return theta_; }
  const GyroBiasFilter& filter() const { return bias_; }

 private:
  GyroBiasFilter bias_;
  double theta_ = 0.0;
};


#endif
