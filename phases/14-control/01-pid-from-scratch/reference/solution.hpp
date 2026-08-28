#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

class Pid {
 public:
  Pid(double kp, double ki, double kd, double min_output, double max_output)
      : kp_(kp), ki_(ki), kd_(kd), min_output_(min_output), max_output_(max_output) {}

  double update(double setpoint, double measurement, double dt) {
    // A time step of zero would divide by zero in the derivative term, and a
    // negative one means the clock went backwards. Neither is a reason to
    // produce a wild command, so hold the previous output.
    if (dt <= 0.0) return last_output_;

    const double error = setpoint - measurement;

    // Derivative on measurement. Using the change in error would make a setpoint
    // change look like an infinitely fast movement and spike the output for one
    // step, which is felt in the hardware as a jolt.
    double derivative = 0.0;
    if (has_last_measurement_) {
      derivative = -(measurement - last_measurement_) / dt;
    }
    last_measurement_ = measurement;
    has_last_measurement_ = true;

    // Work out what the output would be with the integral as it stands, so the
    // anti windup rule below can ask whether accumulating more would help.
    const double unsaturated = kp_ * error + ki_ * integral_ + kd_ * derivative;

    // Anti windup: only accumulate when the output is inside its limits, or
    // when the error would drive it back towards the usable range. Without this
    // the integral grows without bound while the actuator is already flat out,
    // and the controller keeps commanding full power long after the setpoint
    // has dropped.
    const bool saturated_high = unsaturated >= max_output_;
    const bool saturated_low = unsaturated <= min_output_;
    const bool pushing_further = (saturated_high && error > 0.0) || (saturated_low && error < 0.0);
    if (!pushing_further) {
      integral_ += error * dt;
    }

    const double output = kp_ * error + ki_ * integral_ + kd_ * derivative;

    last_output_ = clamp(output);
    return last_output_;
  }

  void reset() {
    integral_ = 0.0;
    last_measurement_ = 0.0;
    last_output_ = 0.0;
    has_last_measurement_ = false;
  }

  double integral() const { return integral_; }
  double lastOutput() const { return last_output_; }

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

#endif  // LESSON_SOLUTION_HPP
