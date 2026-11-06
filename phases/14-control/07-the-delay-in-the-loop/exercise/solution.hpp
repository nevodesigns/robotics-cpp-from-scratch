#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cmath>
#include <cstddef>
#include <limits>
#include <vector>

// A signal arriving some samples late.
//
// Push a value in and the value pushed `samples` calls earlier comes out. Until that
// many have gone in, what comes out is the initial value. With no delay it is a wire.
class DelayLine {
 public:
  explicit DelayLine(std::size_t samples, double initial = 0.0) : buffer_(samples, initial) {}

  double push(double x) {
    // TODO: a ring buffer of `samples` values and a position in it. Read the value
    // at the position, which is the oldest, write x over it, and step on, wrapping
    // at the end. With no samples at all there is nothing to remember, so x comes
    // straight back.
    (void)x;
    (void)head_;
    return 0.0;
  }

  std::size_t samples() const { return buffer_.size(); }

 private:
  std::vector<double> buffer_;
  std::size_t head_ = 0;
};

// The phase a pure delay adds at a frequency, in degrees. A delay is a time, and a
// time at a frequency is a fraction of a turn.
inline double phase_lag_degrees(double delay_seconds, double frequency_hz) {
  // TODO: 360 degrees times the frequency times the delay.
  (void)delay_seconds;
  (void)frequency_hz;
  return 0.0;
}

// The frequency, in radians a second, at which a proportional and derivative
// controller on a pure inertia J has a loop gain of exactly one.
inline double crossover_frequency(double kp, double kd, double inertia) {
  // TODO: the loop gain is (kp + kd s) / (J s^2). Its size at s = j w is
  // sqrt(kp^2 + kd^2 w^2) / (J w^2). Set it to one and square it:
  //
  //   J^2 w^4 - kd^2 w^2 - kp^2 = 0
  //
  // which is a quadratic in w squared with one positive root.
  (void)kp;
  (void)kd;
  (void)inertia;
  return 0.0;
}

// How much more delay the loop could take at its crossover frequency before it becomes
// unstable, as phase in degrees. Negative is already unstable.
inline double phase_margin_degrees(double kp, double kd, double inertia, double delay) {
  // TODO: at crossover the controller leads by atan2(kd w, kp), the inertia lags by
  // exactly 180 degrees, and the delay lags by w times the delay. What is left of the
  // 180 degrees is the margin: lead minus w * delay, in degrees.
  (void)kp;
  (void)kd;
  (void)inertia;
  (void)delay;
  return 0.0;
}

// The margin at one gain, with the derivative gain set for a damping ratio of zeta,
// which is kd = 2 zeta sqrt(kp J).
inline double margin_at_gain(double kp, double inertia, double delay, double zeta) {
  return phase_margin_degrees(kp, 2.0 * zeta * std::sqrt(kp * inertia), inertia, delay);
}

// The largest proportional gain at which the loop is still stable: where the margin
// reaches zero.
inline double largest_stable_gain(double inertia, double delay, double zeta = 1.0) {
  // TODO: a delay of nothing is infinity, since the model has no limit without one.
  // Otherwise search for the gain where margin_at_gain crosses zero. The margin only
  // falls as the gain rises, so halve the range on a log scale: take the geometric
  // mean of the two ends, keep the half that is still stable, and stop when the ends
  // are within a hundredth of a percent. Start from 1e-6 and 1e12, and if even 1e12
  // is stable the answer is infinity.
  (void)inertia;
  (void)delay;
  (void)zeta;
  (void)std::numeric_limits<double>::infinity;
  return 0.0;
}

#endif  // LESSON_SOLUTION_HPP
