#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

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

#endif  // LESSON_SOLUTION_HPP
