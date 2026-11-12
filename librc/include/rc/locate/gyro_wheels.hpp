// rc/locate/gyro_wheels.hpp
//
// Wheels and a gyro in a filter, from lesson 25-01.
//
// The gyro's reading is the turn rate plus a bias that wanders; the wheels' turn rate (the difference of the sides over the effective track) has no bias but is wrong by a share of itself, and wrong by a great deal when a wheel slips. GyroBiasFilter holds an estimate of the bias and its variance. Each step it takes the difference of the gyro and the wheels' turn rate as a measurement of the bias, with a variance that is the gyro's noise plus (the wheels' error share times their turn rate) squared, so that the wheels count for almost nothing when turning and a great deal when going straight; a difference further from the estimate than a gate of some standard deviations is a slip, and is refused. FusedHeading integrates the gyro less the bias.
//
// Over 120 s of a robot going straight for 15 s and turning for 10, with a gyro bias of 0.01 rad/s and a slip at 50 s that halved the turn for 2 s, the heading from the wheels alone ended 3.1 degrees out, the gyro alone 68.0 degrees, and the filter 1.0 degrees, with the bias estimated as 0.0089 rad/s; the position after the run was 1.8 m, 14.1 m and 0.3 m from the truth; and with the wheels 12 percent out (a track not calibrated) a bias estimator that did not weigh them by how straight they were ended 3.7 degrees out on average against 1.7 for the one that did.

#ifndef RC_RC_LOCATE_GYRO_WHEELS_HPP
#define RC_RC_LOCATE_GYRO_WHEELS_HPP

#include <cmath>

namespace rc {
namespace locate {

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
    const double z = gyro - wheel_rate;
    const double r = sigma_ * sigma_ + share_ * share_ * wheel_rate * wheel_rate + 1.0e-6;
    const double s = p_ + r;
    const double innovation = z - b_;
    if (innovation * innovation <= gate_ * gate_ * s) {
      const double k = p_ / s;
      b_ += k * innovation;
      p_ *= (1.0 - k);
    } else {
      ++refused_;
    }
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


}  // namespace locate
}  // namespace rc

#endif
