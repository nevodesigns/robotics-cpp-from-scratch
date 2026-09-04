#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cmath>
#include <cstddef>

#include <rc/core/compat.hpp>

// Why a calibration can fail to exist, rather than quietly being nonsense.
//
// Every one of these is a mistake somebody makes with real references in a real
// workshop, and every one of them produces a division by zero or a meaningless
// line if it is not caught. A fit that cannot be done has to say so.
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
//
// scale and offset are the correction, not the sensor's own behaviour. A sensor
// that reads 0.985 x + 0.42 is corrected by 1.0152 raw - 0.4264, and storing the
// first pair where the second belongs is one of the traps in this lesson: the
// numbers still look plausible and every reading is about twice as wrong as it
// was before you calibrated.
struct Calibration {
  double scale = 1.0;
  double offset = 0.0;

  double apply(double raw) const { return scale * raw + offset; }

  // Two references. The classic workshop calibration: put the sensor at a known
  // near distance, then a known far one.
  //
  // Fit across the range you are going to use. A line through two points close
  // together is exact between them and drifts away from the truth outside them,
  // and how far away is measured in this lesson.
  static rc::expected<Calibration, CalibrationError> from_two_points(
      double raw_a, double true_a, double raw_b, double true_b) {
    const double spread = raw_b - raw_a;
    if (spread == 0.0) return rc::unexpected<CalibrationError>(CalibrationError::no_spread);

    Calibration fit;
    fit.scale = (true_b - true_a) / spread;
    fit.offset = true_a - fit.scale * raw_a;
    return fit;
  }

  // Any number of references, fitted by least squares.
  //
  // The references themselves are measured, so they have their own error, and
  // this is the reason to take more than two: the fit averages that error down
  // by the same square root law that filtering obeys. Four times the references,
  // half the error.
  static rc::expected<Calibration, CalibrationError> from_samples(
      rc::span<const double> raw, rc::span<const double> truth) {
    if (raw.size() != truth.size())
      return rc::unexpected<CalibrationError>(CalibrationError::mismatched_lengths);
    if (raw.size() < 2)
      return rc::unexpected<CalibrationError>(CalibrationError::too_few_points);

    const double n = static_cast<double>(raw.size());
    double sum_r = 0.0, sum_t = 0.0, sum_rr = 0.0, sum_rt = 0.0;
    for (std::size_t i = 0; i < raw.size(); ++i) {
      sum_r += raw[i];
      sum_t += truth[i];
      sum_rr += raw[i] * raw[i];
      sum_rt += raw[i] * truth[i];
    }

    // This denominator is n times the variance of the raw readings. It is zero
    // when they are all the same, and a division by it would produce a nan that
    // then survives most range checks people write. Refuse instead.
    const double denominator = n * sum_rr - sum_r * sum_r;
    if (denominator == 0.0)
      return rc::unexpected<CalibrationError>(CalibrationError::no_spread);

    Calibration fit;
    fit.scale = (n * sum_rt - sum_r * sum_t) / denominator;
    fit.offset = (sum_t - fit.scale * sum_r) / n;
    return fit;
  }

  // How far this line misses the references it was fitted to.
  //
  // Fit it and then ask. A residual around the size of the sensor's noise means
  // the line is as good as the data. A residual much larger than the noise means
  // the sensor is not a straight line over this range, and refitting a line will
  // not help: you need a shorter range, or a curve, or a better sensor.
  double rms_residual(rc::span<const double> raw, rc::span<const double> truth) const {
    if (raw.empty() || raw.size() != truth.size()) return 0.0;

    double sum_squared = 0.0;
    for (std::size_t i = 0; i < raw.size(); ++i) {
      const double error = apply(raw[i]) - truth[i];
      sum_squared += error * error;
    }
    return std::sqrt(sum_squared / static_cast<double>(raw.size()));
  }
};

#endif  // LESSON_SOLUTION_HPP
