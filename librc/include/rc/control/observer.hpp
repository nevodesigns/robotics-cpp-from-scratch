// rc/control/observer.hpp
//
// Speed from a position sensor and a model, from lesson 14-10, graduated.
//
// A position sensor does not report speed, and the obvious way to make it, the difference of two
// readings divided by the time between them, multiplies the sensor's noise by 1 / dt. With a sensor
// good to 0.001 rad read at 1 kHz that is 1.422 rad/s rms against a swing of 3.1 rad/s peak. An
// observer predicts with the commanded acceleration and corrects by the position error, and the same
// readings gave 0.006 rad/s.
//
// The model matters: ignoring the acceleration made a noise free swing lag by 0.543 rad/s where
// using it gave 0.0017. A push the model does not know about leaves a steady speed error of
// 2 d / w, 0.0395 rad/s measured against 0.0400 predicted for 1 rad/s^2 at a bandwidth of 50.
//
// One number sets both gains and it is a trade. Noise only, 0.0002 rad/s at a bandwidth of 5 and 0.1675
// at 400; tracking only, 2.5516 at 5 and 0.0628 at 400; the two together are smallest near 200.

#ifndef RC_CONTROL_OBSERVER_HPP
#define RC_CONTROL_OBSERVER_HPP

namespace rc {
namespace control {

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

}  // namespace control
}  // namespace rc

#endif  // RC_CONTROL_OBSERVER_HPP

