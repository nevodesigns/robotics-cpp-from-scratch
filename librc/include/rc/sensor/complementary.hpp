// rc/sensor/complementary.hpp
//
// A gyro and an accelerometer made into one angle, from lesson 15-11, graduated.
//
// A gyro is smooth and carries a bias; an accelerometer has no bias and is noisy. On a joint standing still, a gyro biased by 0.02 rad/s
// and integrated alone was 0.764 rad rms off over 60 seconds (a ramp predicts 0.757), and the accelerometer alone was 0.050 rad rms off.
// A complementary filter with a time constant of 1 s was 0.020 rad rms off, and on a swinging joint 0.021 against 0.761 and 0.050.
//
// A bias b leaves a steady error of b x tau: 0.0050, 0.0100, 0.0200, 0.0400 and 0.0800 rad for taus of 0.25, 0.5, 1, 2 and 4 s.
// Over the taus tried, the swinging joint's error was 0.0144 at 0.05 s, 0.0082 at 0.2 s, 0.0205 at 1 s and 0.3202 at 20 s.

#ifndef RC_SENSOR_COMPLEMENTARY_HPP
#define RC_SENSOR_COMPLEMENTARY_HPP

namespace rc {
namespace sensor {

// The share of the previous estimate, with the gyro's change added, that the filter keeps each step.
// tau is the crossover time constant in seconds. A tau of zero or less keeps none of it, so the
// accelerometer alone is believed.
inline double complementary_alpha(double tau, double dt) {
  if (tau <= 0.0 || dt <= 0.0) return 0.0;
  return tau / (tau + dt);
}

// The time constant that hands over from the gyro to the accelerometer at `crossover_hz`.
inline double tau_for_crossover(double crossover_hz) {
  if (crossover_hz <= 0.0) return 0.0;
  return 1.0 / (2.0 * 3.14159265358979323846 * crossover_hz);
}

// An angle from a gyro, which is smooth and drifts, and an accelerometer, which is noisy and does not.
class ComplementaryFilter {
 public:
  explicit ComplementaryFilter(double tau) : tau_(tau) {}

  void reset(double angle) { angle_ = angle; }

  // One step: the gyro moves the estimate on, and the accelerometer's angle pulls it back.
  double update(double gyro_rate, double accel_angle, double dt) {
    const double alpha = complementary_alpha(tau_, dt);
    angle_ = alpha * (angle_ + gyro_rate * dt) + (1.0 - alpha) * accel_angle;
    return angle_;
  }

  double angle() const { return angle_; }
  double tau() const { return tau_; }

 private:
  double tau_ = 0.0;
  double angle_ = 0.0;
};

}  // namespace sensor
}  // namespace rc

#endif  // RC_SENSOR_COMPLEMENTARY_HPP
