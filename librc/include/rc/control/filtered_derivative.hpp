// rc/control/filtered_derivative.hpp
//
// A low pass on the derivative term, from lesson 14-11, graduated.
//
// A derivative term differentiates the sensor, and differentiating multiplies noise by 1 / dt. The rms of the
// derivative of a sensor good to 0.001 rad read at 1 kHz was 1.4067 rad/s with no filter, 0.1737 at a time
// constant of 0.005 s, 0.0480 at 0.02 s and 0.0196 at 0.05 s. The price is lag: on a noise free swing the
// derivative's rms error rose from 0 to 0.0697, 0.2769 and 0.6652 rad/s over the same constants.
//
// In a position loop with kp 40 and kd 4 the force changed by 9.373 N step to step with no filter and
// 0.317 N at 0.02 s, and the joint still ended 0.0000 rad from its target.

#ifndef RC_CONTROL_FILTERED_DERIVATIVE_HPP
#define RC_CONTROL_FILTERED_DERIVATIVE_HPP

namespace rc {
namespace control {

// How much of each new raw derivative a first order low pass lets in, from 0 to 1.
//
// tf is the filter's time constant in seconds. Zero or less means no filtering, so everything gets in.
inline double filter_alpha(double tf, double dt) {
  if (dt <= 0.0 || tf <= 0.0) return 1.0;
  return dt / (tf + dt);
}

// The derivative of a measurement, with a low pass on it so that noise does not reach the motor.
class DerivativeFilter {
 public:
  explicit DerivativeFilter(double tf) : tf_(tf) {}

  void reset() {
    has_last_ = false;
    last_ = 0.0;
    value_ = 0.0;
  }

  // The filtered rate of change of `measurement`. The first call has nothing to subtract from, so it gives zero.
  double update(double measurement, double dt) {
    if (dt <= 0.0) return value_;
    if (!has_last_) {
      last_ = measurement;
      has_last_ = true;
      return value_;
    }
    const double raw = (measurement - last_) / dt;
    last_ = measurement;
    value_ += filter_alpha(tf_, dt) * (raw - value_);
    return value_;
  }

  double value() const { return value_; }
  double tf() const { return tf_; }

 private:
  double tf_ = 0.0;
  double last_ = 0.0;
  double value_ = 0.0;
  bool has_last_ = false;
};

}  // namespace control
}  // namespace rc

#endif  // RC_CONTROL_FILTERED_DERIVATIVE_HPP
