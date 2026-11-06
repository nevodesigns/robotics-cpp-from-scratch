#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cmath>
#include <cstddef>
#include <limits>
#include <vector>

// A signal arriving some samples late.
//
// Every real loop has one, in a place it is easy to overlook: the sensor reads a
// moment before it reports, the filter lags, the message crosses a link, the
// command waits for the next cycle. Each is a delay, and they add.
//
// Push a value in and the value pushed `samples` calls earlier comes out. Until
// that many have gone in, what comes out is the initial value, which is the honest
// answer for a signal that has not arrived yet. With no delay it is a wire.
class DelayLine {
 public:
  explicit DelayLine(std::size_t samples, double initial = 0.0) : buffer_(samples, initial) {}

  double push(double x) {
    if (buffer_.empty()) return x;

    const double out = buffer_[head_];
    buffer_[head_] = x;
    head_ = (head_ + 1) % buffer_.size();
    return out;
  }

  std::size_t samples() const { return buffer_.size(); }

 private:
  std::vector<double> buffer_;
  std::size_t head_ = 0;
};

// The phase a pure delay adds at a frequency: a delay is a time, and a time at a
// frequency is a fraction of a turn. It adds no gain at all, which is why it is
// dangerous: nothing about the loop's size changes, and its timing goes wrong.
inline double phase_lag_degrees(double delay_seconds, double frequency_hz) {
  return 360.0 * frequency_hz * delay_seconds;
}

// The frequency, in radians a second, at which a proportional and derivative
// controller on a pure inertia has a loop gain of exactly one.
//
// The loop gain is (kp + kd s) / (J s^2), and its size at s = j w is
// sqrt(kp^2 + kd^2 w^2) / (J w^2). Setting that to one gives a quadratic in w
// squared, which has one positive root.
inline double crossover_frequency(double kp, double kd, double inertia) {
  const double j2 = inertia * inertia;
  const double w2 = (kd * kd + std::sqrt(kd * kd * kd * kd + 4.0 * j2 * kp * kp)) / (2.0 * j2);
  return std::sqrt(w2);
}

// How much more delay the loop could take at its crossover frequency before it
// becomes unstable, expressed as phase, in degrees. Negative is already unstable.
//
// The controller leads by the angle of kp + j w kd, the inertia is two integrators
// and lags by exactly 180 degrees, and the delay lags by w times the delay. What is
// left of the 180 degrees is the margin.
inline double phase_margin_degrees(double kp, double kd, double inertia, double delay) {
  const double w = crossover_frequency(kp, kd, inertia);
  const double lead = std::atan2(kd * w, kp);
  return (lead - w * delay) * 180.0 / 3.141592653589793;
}

// The largest proportional gain at which the loop is still stable, with the
// derivative gain set for a damping ratio of zeta, found by searching for the gain
// where the margin reaches zero.
//
// A delay of nothing is infinity. The model has no limit without one, which is a
// statement about the model: a real loop is still limited by its sampling, and the
// caller should include half a sample period in the delay, which is what holding a
// command between samples costs.
inline double margin_at_gain(double kp, double inertia, double delay, double zeta) {
  return phase_margin_degrees(kp, 2.0 * zeta * std::sqrt(kp * inertia), inertia, delay);
}

inline double largest_stable_gain(double inertia, double delay, double zeta = 1.0) {
  if (delay <= 0.0) return std::numeric_limits<double>::infinity();

  double lo = 1e-6;
  double hi = 1e12;
  if (margin_at_gain(hi, inertia, delay, zeta) > 0.0) return std::numeric_limits<double>::infinity();

  // The margin only falls as the gain rises, so halving the range on a log scale
  // finds where it crosses zero.
  while (hi / lo > 1.0001) {
    const double mid = std::sqrt(lo * hi);
    if (margin_at_gain(mid, inertia, delay, zeta) > 0.0) lo = mid;
    else hi = mid;
  }
  return lo;
}

#endif  // LESSON_SOLUTION_HPP
