#include <rc/test/rc_test.hpp>

#include <cmath>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <vector>

#include "solution.hpp"

namespace {

// Noise that is the same number on every compiler.
//
// std::normal_distribution is not specified to produce the same sequence from
// the same seed on two implementations, so a table printed from it says one
// thing on Linux and another on Windows. This is integer arithmetic and
// division, which are exact and identical everywhere, so the numbers printed
// below are the numbers in the lesson text whichever toolchain you build with.
//
// Twelve uniforms added together and centred is close enough to a gaussian for
// a sensor model: it has mean 0 and standard deviation 1 by construction.
class Noise {
 public:
  explicit Noise(std::uint64_t seed) : state_(seed * 6364136223846793005ULL + 1ULL) {}

  double uniform() {
    state_ = state_ * 6364136223846793005ULL + 1442695040888963407ULL;
    return static_cast<double>((state_ >> 11) & 0x1FFFFFFFFFFFFFULL) /
           static_cast<double>(0x20000000000000ULL);
  }

  double gaussian() {
    double total = 0.0;
    for (int i = 0; i < 12; ++i) total += uniform();
    return total - 6.0;
  }

 private:
  std::uint64_t state_;
};

// A rangefinder that is wrong in three separate ways: it reads 1.5 percent
// short, it reads 42 cm long, it bends very slightly at distance, and it
// jitters. Only the last of those is noise.
class Rangefinder {
 public:
  explicit Rangefinder(double sigma = 0.0, double curve = 0.0, std::uint64_t seed = 7)
      : sigma_(sigma), curve_(curve), noise_(seed) {}

  double read(double truth) {
    const double clean = kScale * truth + kOffset + curve_ * truth * truth;
    return sigma_ > 0.0 ? clean + sigma_ * noise_.gaussian() : clean;
  }

  static constexpr double kScale = 0.985;
  static constexpr double kOffset = 0.42;

 private:
  double sigma_;
  double curve_;
  Noise noise_;
};

// The average of n readings of the same true distance.
double average_of(int n, double truth) {
  Rangefinder sensor(0.05);
  double total = 0.0;
  for (int i = 0; i < n; ++i) total += sensor.read(truth);
  return total / static_cast<double>(n);
}

Calibration must_fit(rc::span<const double> raw, rc::span<const double> truth) {
  const auto fit = Calibration::from_samples(raw, truth);
  return fit.has_value() ? fit.value() : Calibration{};
}

}  // namespace

RC_TEST("two references define the correction exactly, when the sensor is a line") {
  Rangefinder sensor;
  const auto fit = Calibration::from_two_points(sensor.read(1.0), 1.0, sensor.read(9.0), 9.0);
  RC_REQUIRE(fit.has_value());

  // The correction is the sensor's own line turned around, not a copy of it.
  RC_CHECK_NEAR(fit.value().scale, 1.0 / Rangefinder::kScale, 1e-12);
  RC_CHECK_NEAR(fit.value().offset, -Rangefinder::kOffset / Rangefinder::kScale, 1e-12);

  // And it is right everywhere, including well outside the two points, because
  // this sensor really is a straight line.
  for (const double truth : {0.0, 2.5, 20.0, 100.0})
    RC_CHECK_NEAR(fit.value().apply(sensor.read(truth)), truth, 1e-9);
}

RC_TEST("a fit that cannot be done says so rather than dividing") {
  const auto flat = Calibration::from_two_points(4.0, 1.0, 4.0, 9.0);
  RC_REQUIRE(!flat.has_value());
  RC_CHECK(flat.error() == CalibrationError::no_spread);

  const double raw[] = {1.0, 2.0, 3.0};
  const double truth[] = {1.0, 2.0};
  const auto ragged = Calibration::from_samples(raw, truth);
  RC_REQUIRE(!ragged.has_value());
  RC_CHECK(ragged.error() == CalibrationError::mismatched_lengths);

  const double one_raw[] = {1.0};
  const double one_truth[] = {1.0};
  const auto lonely = Calibration::from_samples(one_raw, one_truth);
  RC_REQUIRE(!lonely.has_value());
  RC_CHECK(lonely.error() == CalibrationError::too_few_points);

  const double same[] = {7.0, 7.0, 7.0, 7.0, 7.0};
  const double moving[] = {7.0, 8.0, 9.0, 10.0, 11.0};
  const auto stuck = Calibration::from_samples(same, moving);
  RC_REQUIRE(!stuck.has_value());
  RC_CHECK(stuck.error() == CalibrationError::no_spread);
}

RC_TEST("the nan that refusal exists to prevent") {
  // What from_samples would have returned had it divided anyway. The sums are
  // accumulated from the same arrays the fit would have seen, rather than
  // written as literals: MSVC folds a constant division by zero at compile time
  // and refuses to build it, with error C2124, which is a reasonable thing for
  // a compiler to do and also means the only portable way to reach this value
  // is to compute it the way the program actually would.
  std::vector<double> raw, truth;
  for (int i = 0; i < 5; ++i) {
    raw.push_back(7.0);
    truth.push_back(7.0 + i);
  }

  double sum_r = 0.0, sum_t = 0.0, sum_rr = 0.0, sum_rt = 0.0;
  for (std::size_t i = 0; i < raw.size(); ++i) {
    sum_r += raw[i];
    sum_t += truth[i];
    sum_rr += raw[i] * raw[i];
    sum_rt += raw[i] * truth[i];
  }

  const double denominator = 5.0 * sum_rr - sum_r * sum_r;
  const double numerator = 5.0 * sum_rt - sum_r * sum_t;
  RC_CHECK_EQ(denominator, 0.0);
  RC_CHECK_EQ(numerator, 0.0);

  // Zero over zero is a nan rather than an infinity, which matters: an infinity
  // at least compares larger than everything.
  const double scale = numerator / denominator;
  RC_CHECK(std::isnan(scale));

  // And here is why that matters more than a wrong number would. These two
  // range checks are the same sentence in English. They are not the same
  // sentence in C++, because every comparison against a nan is false.
  const double reading = scale * 7.0;
  RC_CHECK(!(reading < 0.0 || reading > 100.0));    // lets the nan straight through
  RC_CHECK(!(reading >= 0.0 && reading <= 100.0));  // catches it

  // A nan in a controller is not a large number, it is the end of the loop:
  // it spreads into the integrator and stays.
  RC_CHECK(std::isnan(reading + 1.0));
  RC_CHECK(std::isnan(reading * 0.0));
}

RC_TEST("averaging drives the noise to nothing and leaves the bias untouched") {
  std::cout << "\n    the sensor reads " << Rangefinder::kScale << " x + "
            << Rangefinder::kOffset << ", and the truth is 10 m\n\n";
  std::cout << "    " << std::left << std::setw(12) << "samples" << std::right
            << std::setw(12) << "estimate" << std::setw(12) << "error"
            << std::setw(14) << "noise term" << "\n";

  double last_error = 0.0;
  for (const int samples : {1, 10, 100, 10000, 1000000}) {
    const double estimate = average_of(samples, 10.0);
    last_error = estimate - 10.0;
    std::cout << "    " << std::left << std::setw(12) << samples << std::right
              << std::fixed << std::setprecision(4) << std::setw(12) << estimate
              << std::setw(12) << last_error << std::setw(14)
              << 0.05 / std::sqrt(static_cast<double>(samples)) << "\n";
  }

  // A million readings. The noise is down to half a millimetre and the answer
  // is still 27 cm wrong, because the bias was never a random thing to average.
  const double bias = (Rangefinder::kScale - 1.0) * 10.0 + Rangefinder::kOffset;
  RC_CHECK_NEAR(bias, 0.27, 1e-12);
  RC_CHECK_NEAR(last_error, bias, 0.001);

  // Averaging a hundred readings and averaging a million are the same answer.
  RC_CHECK_NEAR(average_of(1000000, 10.0), average_of(100, 10.0), 0.005);
}

RC_TEST("one reference is exact where you tested it and wrong at both ends") {
  Rangefinder sensor;
  const double zero_at = 10.0;
  const double correction = zero_at - sensor.read(zero_at);

  std::cout << "\n    zeroed at 10 m\n\n";
  std::cout << "    " << std::right << std::setw(9) << "truth" << std::setw(10)
            << "raw" << std::setw(12) << "corrected" << std::setw(10) << "error"
            << "\n";

  double error_at_zero = 0.0, error_at_twenty = 0.0;
  for (const double truth : {0.0, 5.0, 10.0, 15.0, 20.0}) {
    const double corrected = sensor.read(truth) + correction;
    if (truth == 0.0) error_at_zero = corrected - truth;
    if (truth == 20.0) error_at_twenty = corrected - truth;
    std::cout << "    " << std::fixed << std::setprecision(3) << std::setw(9)
              << truth << std::setw(10) << sensor.read(truth) << std::setw(12)
              << corrected << std::setw(10) << corrected - truth << "\n";
  }

  // Perfect at the one place it was checked.
  RC_CHECK_NEAR(sensor.read(zero_at) + correction, zero_at, 1e-12);

  // And wrong at both ends, in opposite directions, which is the signature of
  // a scale error that an offset was asked to fix. Same size, opposite sign.
  RC_CHECK_NEAR(error_at_zero, 0.15, 1e-9);
  RC_CHECK_NEAR(error_at_twenty, -0.15, 1e-9);
  RC_CHECK(error_at_zero * error_at_twenty < 0.0);
}

RC_TEST("a calibration applied backwards is worse than none at all") {
  Rangefinder sensor;
  const auto fit = Calibration::from_two_points(sensor.read(0.0), 0.0, sensor.read(20.0), 20.0);
  RC_REQUIRE(fit.has_value());

  // The sensor's own coefficients, stored where the correction belongs. This is
  // what a datasheet gives you, and it is the wrong direction.
  Calibration backwards;
  backwards.scale = Rangefinder::kScale;
  backwards.offset = Rangefinder::kOffset;

  std::cout << "\n    " << std::right << std::setw(9) << "truth" << std::setw(12)
            << "corrected" << std::setw(12) << "backwards" << std::setw(14)
            << "uncorrected" << "\n";

  for (const double truth : {0.0, 5.0, 10.0, 20.0}) {
    const double raw = sensor.read(truth);
    std::cout << "    " << std::fixed << std::setprecision(3) << std::setw(9)
              << truth << std::setw(12) << fit.value().apply(raw) - truth
              << std::setw(12) << backwards.apply(raw) - truth << std::setw(14)
              << raw - truth << "\n";
  }

  // The numbers it produces are plausible. They are also further from the truth
  // than the raw reading was, at every distance.
  for (const double truth : {0.0, 5.0, 10.0, 20.0}) {
    const double raw = sensor.read(truth);
    RC_CHECK(std::fabs(backwards.apply(raw) - truth) > std::fabs(raw - truth));
    RC_CHECK_NEAR(fit.value().apply(raw), truth, 1e-9);
  }
}

RC_TEST("a line fitted over two metres is not a line over twenty") {
  // Now with a slight bend, which every real sensor has somewhere.
  Rangefinder curved(0.0, 0.0009);
  const auto wide = Calibration::from_two_points(curved.read(0.0), 0.0, curved.read(20.0), 20.0);
  const auto narrow = Calibration::from_two_points(curved.read(0.0), 0.0, curved.read(2.0), 2.0);
  RC_REQUIRE(wide.has_value());
  RC_REQUIRE(narrow.has_value());

  std::cout << "\n    a sensor with a slight bend, fitted two ways\n\n";
  std::cout << "    " << std::right << std::setw(9) << "truth" << std::setw(16)
            << "fitted 0..20" << std::setw(15) << "fitted 0..2" << "\n";

  for (const double truth : {1.0, 5.0, 10.0, 20.0, 40.0}) {
    const double raw = curved.read(truth);
    std::cout << "    " << std::fixed << std::setprecision(3) << std::setw(9)
              << truth << std::setw(16) << wide.value().apply(raw) - truth
              << std::setw(15) << narrow.value().apply(raw) - truth << "\n";
  }

  // Inside the range it was fitted over, the wide fit is off by 9 cm at worst,
  // which is the bend and not the fit: a line cannot follow a curve.
  RC_CHECK_NEAR(wide.value().apply(curved.read(10.0)) - 10.0, -0.090, 0.002);

  // The narrow fit is excellent where it was made and then leaves.
  RC_CHECK(std::fabs(narrow.value().apply(curved.read(2.0)) - 2.0) < 0.001);
  RC_CHECK(std::fabs(narrow.value().apply(curved.read(20.0)) - 20.0) > 0.3);
  RC_CHECK(std::fabs(narrow.value().apply(curved.read(40.0)) - 40.0) > 1.3);

}

RC_TEST("four times the references, half the error") {
  std::cout << "\n    least squares over noisy references, rms error over\n";
  std::cout << "    0 to 20 m, averaged over 40 trials\n\n";
  std::cout << "    " << std::right << std::setw(12) << "references"
            << std::setw(14) << "rms error" << std::setw(20) << "of the previous" << "\n";

  std::vector<double> results;
  double previous = 0.0;
  for (const int references : {2, 8, 32, 128}) {
    double total = 0.0;
    const int trials = 40;
    for (int trial = 0; trial < trials; ++trial) {
      Rangefinder sensor(0.05, 0.0, static_cast<std::uint64_t>(31 + trial));
      std::vector<double> raw, truth;
      for (int i = 0; i < references; ++i) {
        const double t = 20.0 * i / (references - 1);
        truth.push_back(t);
        raw.push_back(sensor.read(t));
      }
      const Calibration fit = must_fit(raw, truth);

      Rangefinder clean;
      double sum_squared = 0.0;
      int count = 0;
      for (double t = 0.0; t <= 20.0; t += 0.5) {
        const double error = fit.apply(clean.read(t)) - t;
        sum_squared += error * error;
        ++count;
      }
      total += std::sqrt(sum_squared / count);
    }
    const double rms = total / trials;
    results.push_back(rms);
    std::cout << "    " << std::right << std::setw(12) << references << std::fixed
              << std::setprecision(4) << std::setw(14) << rms << std::setw(20)
              << (previous > 0.0 ? rms / previous : 1.0) << "\n";
    previous = rms;
  }

  std::cout << "\n    the same square root law as the filter in 15-01, spent on\n";
  std::cout << "    the references rather than on the readings\n";

  // Each step is four times the references, and each one roughly halves the
  // error. Loose bounds, because forty trials is forty trials.
  for (std::size_t i = 1; i < results.size(); ++i) {
    RC_CHECK(results[i] < results[i - 1] * 0.62);
    RC_CHECK(results[i] > results[i - 1] * 0.38);
  }

  // Two references and a hundred and twenty eight are not the same calibration.
  RC_CHECK(results.front() > results.back() * 5.0);
}

RC_TEST("the residual finds a bend only when the bend is bigger than the noise") {
  // Two sensors with identical noise. One is straight and one bends by nine
  // hundredths of a percent of the range, which is not visible in any single
  // reading and is not something a longer average will help with.
  //
  // The residual is the only number that can tell them apart. Whether it does
  // is a question about the noise, so ask it at two noise levels.
  std::cout << "\n    rms residual, 41 references over 0 to 20 m\n\n";
  std::cout << "    " << std::right << std::setw(12) << "noise" << std::setw(14)
            << "straight" << std::setw(12) << "bent" << std::setw(14) << "ratio" << "\n";

  double ratio_noisy = 0.0, ratio_quiet = 0.0;
  for (const double sigma : {0.05, 0.005}) {
    Rangefinder straight(sigma, 0.0, 3);
    Rangefinder bent(sigma, 0.0009, 3);

    std::vector<double> raw_straight, raw_bent, truth;
    for (double t = 0.0; t <= 20.0; t += 0.5) {
      truth.push_back(t);
      raw_straight.push_back(straight.read(t));
      raw_bent.push_back(bent.read(t));
    }

    const double flat = must_fit(raw_straight, truth).rms_residual(raw_straight, truth);
    const double curved = must_fit(raw_bent, truth).rms_residual(raw_bent, truth);
    (sigma > 0.01 ? ratio_noisy : ratio_quiet) = curved / flat;

    std::cout << "    " << std::fixed << std::setprecision(4) << std::setw(12) << sigma
              << std::setw(14) << flat << std::setw(12) << curved
              << std::setw(14) << std::setprecision(2) << curved / flat << "\n";
  }

  // At five centimetres of noise the bend is 11 percent of the residual, which
  // is nothing: two runs of the same straight sensor differ by more than that.
  // The fit succeeded, the residual looked ordinary, and the sensor is bent.
  RC_CHECK(ratio_noisy < 1.2);

  // Quiet the sensor by a factor of ten and the same bend stands out by five.
  // What the residual can find is set by what the noise hides, so the number to
  // compare it against is the sensor's noise, not zero.
  RC_CHECK(ratio_quiet > 3.0);
}
