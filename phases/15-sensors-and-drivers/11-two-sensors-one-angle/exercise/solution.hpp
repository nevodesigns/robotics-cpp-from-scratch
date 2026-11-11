#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

// The share of the previous estimate, with the gyro's change added, that the filter keeps each step.
inline double complementary_alpha(double tau, double dt) {
  // TODO: tau is the crossover time constant in seconds. The share is tau / (tau + dt).
  // A tau or dt of zero or less keeps none of it: return 0.
  (void)tau;
  (void)dt;
  return 1.0;
}

// The time constant that hands over from the gyro to the accelerometer at `crossover_hz`.
inline double tau_for_crossover(double crossover_hz) {
  // TODO: 1 / (2 pi f). Zero for a frequency of zero or less.
  (void)crossover_hz;
  return 0.0;
}

// An angle from a gyro, which is smooth and drifts, and an accelerometer, which is noisy and does not.
class ComplementaryFilter {
 public:
  explicit ComplementaryFilter(double tau) : tau_(tau) {}

  void reset(double angle) { angle_ = angle; }

  // One step: the gyro moves the estimate on, and the accelerometer's angle pulls it back.
  double update(double gyro_rate, double accel_angle, double dt) {
    // TODO: alpha = complementary_alpha(tau_, dt);
    // angle_ = alpha * (angle_ + gyro_rate * dt) + (1 - alpha) * accel_angle; return angle_.
    (void)gyro_rate;
    (void)accel_angle;
    (void)dt;
    return angle_;
  }

  double angle() const { return angle_; }
  double tau() const { return tau_; }

 private:
  double tau_ = 0.0;
  double angle_ = 0.0;
};

#endif  // LESSON_SOLUTION_HPP
