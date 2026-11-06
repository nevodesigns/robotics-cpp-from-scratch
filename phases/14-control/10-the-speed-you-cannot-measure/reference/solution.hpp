#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

// How hard the observer leans on each measurement.
struct ObserverGains {
  double position = 0.0;   // per second, applied to the position error
  double velocity = 0.0;   // per second squared, applied to the position error
};

// Gains that put both observer poles at minus `bandwidth`, in radians a second.
//
// A critically damped pair: the estimate settles as fast as the bandwidth allows and does not ring.
// The two numbers are 2 w and w squared, which is what (s + w)^2 expands to.
inline ObserverGains observer_gains(double bandwidth) {
  ObserverGains gains;
  if (bandwidth <= 0.0) return gains;
  gains.position = 2.0 * bandwidth;
  gains.velocity = bandwidth * bandwidth;
  return gains;
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
    const double predicted_position = position_ + velocity_ * dt + 0.5 * acceleration * dt * dt;
    const double predicted_velocity = velocity_ + acceleration * dt;
    const double error = measured - predicted_position;
    position_ = predicted_position + gains_.position * dt * error;
    velocity_ = predicted_velocity + gains_.velocity * dt * error;
  }

  double position() const { return position_; }
  double velocity() const { return velocity_; }

 private:
  ObserverGains gains_;
  double position_ = 0.0;
  double velocity_ = 0.0;
};

#endif  // LESSON_SOLUTION_HPP
