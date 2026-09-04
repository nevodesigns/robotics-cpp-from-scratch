#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cmath>
#include <cstddef>

#include <rc/core/compat.hpp>

// Why a calibration can fail to exist, rather than quietly being nonsense.
enum class CalibrationError {
  too_few_points,      // one reference defines a point, not a line
  mismatched_lengths,  // a raw reading with no truth beside it
  no_spread,           // every reference read the same value
};

inline const char* describe(CalibrationError error) {
  switch (error) {
    case CalibrationError::too_few_points:
      return "a line needs at least two references";
    case CalibrationError::mismatched_lengths:
      return "there are not as many true values as raw readings";
    case CalibrationError::no_spread:
      return "every reference read the same value, so there is no slope to find";
  }
  return "unknown";
}

// The straight line from what the sensor reports to what is true.
struct Calibration {
  double scale = 1.0;
  double offset = 0.0;

  double apply(double raw) const { return scale * raw + offset; }

  // TODO 1: fit a line through two references.
  //
  // You are solving for the correction, not describing the sensor. Given that
  // the sensor read raw_a where the truth was true_a, and raw_b where it was
  // true_b, find the scale and offset such that
  //
  //     scale * raw_a + offset == true_a
  //     scale * raw_b + offset == true_b
  //
  // Two equations, two unknowns. Subtract one from the other to get the scale,
  // then put it back into either one to get the offset.
  //
  // Return CalibrationError::no_spread if the two raw readings are equal, which
  // is a horizontal pair of points with no line through them.
  static rc::expected<Calibration, CalibrationError> from_two_points(
      double raw_a, double true_a, double raw_b, double true_b) {
    (void)raw_a;
    (void)true_a;
    (void)raw_b;
    (void)true_b;
    return Calibration{};
  }

  // TODO 2: fit a line through any number of references by least squares.
  //
  // With n pairs, accumulate four sums: of the raw readings, of the true
  // values, of raw squared, and of raw times true. Then
  //
  //     denominator = n * sum_rr - sum_r * sum_r
  //     scale       = (n * sum_rt - sum_r * sum_t) / denominator
  //     offset      = (sum_t - scale * sum_r) / n
  //
  // Refuse the fit rather than dividing, in three cases: the two spans are
  // different lengths, there are fewer than two references, and the denominator
  // is zero. That last one is what happens when every reference read the same
  // value, and it is the important one: dividing anyway gives you a nan, and a
  // nan then passes most of the range checks people write.
  static rc::expected<Calibration, CalibrationError> from_samples(
      rc::span<const double> raw, rc::span<const double> truth) {
    (void)raw;
    (void)truth;
    return Calibration{};
  }

  // TODO 3: report how far this line misses the references it was fitted to.
  //
  // The root mean square of apply(raw[i]) - truth[i]. Return 0.0 if there is
  // nothing to measure, or if the two spans disagree about how many there are.
  //
  // This is the number that tells you whether a straight line was ever the
  // right shape. Compare it against the sensor's noise: much larger means the
  // sensor is curved, and no amount of refitting a line will fix that.
  double rms_residual(rc::span<const double> raw, rc::span<const double> truth) const {
    (void)raw;
    (void)truth;
    return 0.0;
  }
};

#endif  // LESSON_SOLUTION_HPP
