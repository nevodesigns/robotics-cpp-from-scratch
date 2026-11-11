#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

// How much of each new raw derivative a first order low pass lets in, from 0 to 1.
inline double filter_alpha(double tf, double dt) {
  // TODO: tf is the time constant in seconds. The share is dt / (tf + dt).
  // Zero or less for dt or tf means no filtering, so everything gets in: return 1.
  (void)tf;
  (void)dt;
  return 0.0;
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

  // The filtered rate of change of `measurement`.
  double update(double measurement, double dt) {
    // TODO: three things.
    // 1. If dt is zero or less, return the value as it stands.
    // 2. The first call has nothing to subtract from: remember the measurement, set has_last_, return value_.
    // 3. Otherwise raw = (measurement - last_) / dt, remember the measurement, and move value_
    //    toward raw by filter_alpha(tf_, dt): value_ += alpha * (raw - value_). Return value_.
    (void)measurement;
    (void)dt;
    (void)has_last_;
    (void)last_;
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
