#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

class Pid {
 public:
  Pid(double kp, double ki, double kd, double min_output, double max_output)
      : kp_(kp), ki_(ki), kd_(kd), min_output_(min_output), max_output_(max_output) {}

  // Returns the command for this time step.
  //
  // Requirements, all four of them tested:
  //   1. output is clamped between min_output and max_output
  //   2. the integral does not accumulate while the output is saturated and the
  //      error pushes it further into saturation
  //   3. the derivative term uses the change in measurement, negated, not the
  //      change in error
  //   4. a dt of zero or less returns the previous output unchanged
  double update(double setpoint, double measurement, double dt) {
    // TODO
    (void)setpoint;
    (void)measurement;
    (void)dt;
    return 0.0;
  }

  void reset() {
    // TODO: clear the accumulated state so the next update starts clean.
  }

  double integral() const { return integral_; }
  double lastOutput() const { return last_output_; }

 private:
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
