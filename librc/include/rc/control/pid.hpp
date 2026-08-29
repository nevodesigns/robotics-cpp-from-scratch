// rc/control/pid.hpp
//
// The controller from lesson 14-01, graduated.
//
// Two details separate this from a textbook PID, and both were the subject of
// that lesson: the integral does not accumulate while the output is saturated
// and the error would push it further into saturation, and the derivative is
// taken on the measurement rather than on the error so that changing the
// setpoint does not jolt the actuator.

#ifndef RC_CONTROL_PID_HPP
#define RC_CONTROL_PID_HPP

namespace rc {
namespace control {

class Pid {
 public:
  Pid(double kp, double ki, double kd, double min_output, double max_output)
      : kp_(kp), ki_(ki), kd_(kd), min_output_(min_output), max_output_(max_output) {}

  double update(double setpoint, double measurement, double dt) {
    // A non positive step carries no information about elapsed time, and would
    // divide by zero in the derivative. Holding the previous output is the only
    // safe answer.
    if (dt <= 0.0) return last_output_;

    const double error = setpoint - measurement;

    double derivative = 0.0;
    if (has_last_measurement_) derivative = -(measurement - last_measurement_) / dt;
    last_measurement_ = measurement;
    has_last_measurement_ = true;

    const double unsaturated = kp_ * error + ki_ * integral_ + kd_ * derivative;

    const bool high = unsaturated >= max_output_;
    const bool low = unsaturated <= min_output_;
    const bool pushing_further = (high && error > 0.0) || (low && error < 0.0);
    if (!pushing_further) integral_ += error * dt;

    last_output_ = clamp(kp_ * error + ki_ * integral_ + kd_ * derivative);
    return last_output_;
  }

  void reset() {
    integral_ = 0.0;
    last_measurement_ = 0.0;
    last_output_ = 0.0;
    has_last_measurement_ = false;
  }

  double integral() const { return integral_; }
  double last_output() const { return last_output_; }

 private:
  double clamp(double value) const {
    if (value < min_output_) return min_output_;
    if (value > max_output_) return max_output_;
    return value;
  }

  double kp_ = 0.0;
  double ki_ = 0.0;
  double kd_ = 0.0;
  double min_output_ = -1.0;
  double max_output_ = 1.0;

  double integral_ = 0.0;
  double last_measurement_ = 0.0;
  double last_output_ = 0.0;
  bool has_last_measurement_ = false;
};

}  // namespace control
}  // namespace rc

#endif  // RC_CONTROL_PID_HPP
