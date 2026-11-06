#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

// How hard the observer leans on each measurement.
struct ObserverGains {
  double position = 0.0;   // per second, applied to the position error
  double velocity = 0.0;   // per second squared, applied to the position error
};

// Gains that put both observer poles at minus `bandwidth`, in radians a second.
inline ObserverGains observer_gains(double bandwidth) {
  // TODO: a critically damped pair, (s + w)^2 = s^2 + 2 w s + w^2.
  // position gain = 2 * bandwidth, velocity gain = bandwidth * bandwidth.
  // A bandwidth of zero or less gives both gains zero.
  (void)bandwidth;
  return ObserverGains{};
}

// Position and speed from a position sensor, using the acceleration that was commanded.
class VelocityObserver {
 public:
  explicit VelocityObserver(const ObserverGains& gains) : gains_(gains) {}

  void reset(double position, double velocity) {
    position_ = position;
    velocity_ = velocity;
  }

  // One step: predict from the model, then correct by the position error.
  void update(double measured, double acceleration, double dt) {
    // TODO: two steps.
    //
    // Predict: the position moves on by velocity * dt + 0.5 * acceleration * dt * dt, and the
    // velocity by acceleration * dt.
    //
    // Correct: error = measured - predicted position. Add gains_.position * dt * error to the
    // position and gains_.velocity * dt * error to the velocity.
    (void)measured;
    (void)acceleration;
    (void)dt;
    (void)gains_;
  }

  double position() const { return position_; }
  double velocity() const { return velocity_; }

 private:
  ObserverGains gains_;
  double position_ = 0.0;
  double velocity_ = 0.0;
};

#endif  // LESSON_SOLUTION_HPP
