// rc/core/calibration.hpp
//
// The straight line from what a sensor reports to what is true, from lesson
// 15-02.
//
// Filtering and calibration answer two different questions and are constantly
// confused for each other. A filter removes the part of the error that is
// random, and rc/core/filters.hpp says what that costs. Nothing in this file
// touches noise: it removes the part of the error that is not random, which no
// amount of averaging will ever reach.
//
// Measured on a sensor reading 0.985 x + 0.42 with 5 cm of jitter, at a true
// 10 m: averaging one reading is 31 cm out, averaging a million is 27 cm out.
// Past about a hundred samples nothing further happens. If more samples stop
// helping, what is left is not noise.

#ifndef RC_CORE_CALIBRATION
#define RC_CORE_CALIBRATION

#include <cmath>
#include <cstddef>

#include <rc/core/compat.hpp>

namespace rc {
namespace core {

// Why a calibration can fail to exist, rather than quietly being nonsense.
//
// Each of these is a division by zero waiting to happen, and the last one is
// worth more than the other two: it produces a nan rather than an infinity, and
// a nan passes any range check written as a list of things to reject, because
// every comparison against it is false.
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

// scale and offset are the correction, not the sensor's own behaviour.
//
// A sensor that reads 0.985 x + 0.42 is corrected by 1.0152 raw - 0.4264.
// Storing the first pair where the second belongs applies the error a second
// time instead of undoing it, and leaves every reading about twice as wrong as
// doing nothing, in numbers that still look plausible.
struct Calibration {
  double scale = 1.0;
  double offset = 0.0;

  double apply(double raw) const { return scale * raw + offset; }

  // Two references, taken across the range the robot actually works over.
  //
  // Two is the minimum that can find both the scale and the offset, and one is
  // not enough for a reason that is easy to miss: zeroing at a single distance
  // is exact there and wrong at both ends in opposite directions.
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
  // The references are themselves measured, so they carry their own error, and
  // the fit averages it down by the same square root law a filter obeys: four
  // times the references, half the error. Measured, over 0 to 20 m with 5 cm of
  // noise, the rms error of the resulting calibration is 0.0372 m from two
  // references and 0.0052 m from a hundred and twenty eight.
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

    // n times the variance of the raw readings, and zero when they are all the
    // same: a reference that was never moved, a sensor that saturated, a driver
    // handing back its last value. The numerator is zero as well, so dividing
    // gives 0.0 / 0.0, which is a nan. Refuse instead.
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
  // Compare it against the sensor's noise, not against zero. A residual around
  // the size of the noise means the line is as good as the data. A residual
  // well above it means the sensor is not straight over this range, and
  // refitting a line will not help.
  //
  // What this can find is set by what the noise hides. The same bend that shows
  // up as six times the residual on a sensor with 5 mm of noise is 11 percent
  // on the same sensor with 5 cm, which is to say invisible. The bend is still
  // there either way.
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

}  // namespace core
}  // namespace rc

#endif  // RC_CORE_CALIBRATION
