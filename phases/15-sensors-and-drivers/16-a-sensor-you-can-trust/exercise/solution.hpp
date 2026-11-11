#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cmath>
#include <cstddef>
#include <cstdint>

#include <rc/core/calibration.hpp>
#include <rc/core/outlier.hpp>
#include <rc/core/smoother.hpp>

// What the pipeline does, and with what numbers.
struct PipelineConfig {
  rc::core::Calibration calibration;   // scale and offset, applied first
  bool use_hampel = true;              // reject outliers after calibrating
  std::size_t window = 7;              // the rejection filter's window
  double threshold = 3.0;              // in robust standard deviations
  double min_scale = 0.05;             // the smallest spread it will trust, in the sensor's units
  double alpha = 0.2;                  // the smoother: the share of each new reading let in
};

// Calibrate, then throw out the wild readings, then smooth. In that order.
class Pipeline {
 public:
  explicit Pipeline(const PipelineConfig& config)
      : config_(config), hampel_(config.window, config.threshold, config.min_scale), smoother_(config.alpha) {}

  double update(double raw) {
    // TODO: three stages, in this order.
    // 1. calibrated = config_.calibration.apply(raw)
    // 2. clean = hampel_.update(calibrated), but only if config_.use_hampel; otherwise clean = calibrated
    // 3. return smoother_.update(clean)
    (void)raw;
    (void)hampel_;
    (void)smoother_;
    (void)config_;
    return 0.0;
  }

 private:
  PipelineConfig config_;
  rc::core::HampelFilter hampel_;
  rc::core::Smoother smoother_;
};

namespace detail {
struct Noise {
  std::uint32_t state = 17;
  std::uint32_t next() {
    state ^= state << 13;
    state ^= state >> 17;
    state ^= state << 5;
    return state;
  }
  double uniform() { return static_cast<double>(next() >> 8) / 16777216.0; }
  double gaussian() {
    double sum = 0.0;
    for (int i = 0; i < 12; ++i) sum += uniform();
    return sum - 6.0;
  }
};
}  // namespace detail

// How much of the sensor's noise is left: the rms of the output over the rms of the input, for a steady signal
// with noise of standard deviation 1 on it. 5000 readings, the first 200 left out.
inline double measure_noise_ratio(const PipelineConfig& config) {
  // TODO: feed 5000 readings of noise (detail::Noise, noise.gaussian()) to a Pipeline. Skip the first 200 outputs.
  // Add up raw squared and output squared over the rest; return sqrt(out / in). Zero if nothing was counted.
  (void)config;
  return -1.0;
}

// How far behind a ramp of one unit per second the output runs, in seconds, once it has settled:
// 1500 readings at `dt` seconds, the lag read from the last 100.
inline double measure_ramp_lag(const PipelineConfig& config, double dt) {
  // TODO: feed t = i * dt for i up to 1500 to a Pipeline. Average (t - output) over the last 100 readings.
  (void)config;
  (void)dt;
  return -1.0;
}

// How long after a step from 0 to 1 the output first reaches 0.5, in seconds, with no noise. The step comes at
// reading 100. Negative if it has not got there after 2000 readings.
inline double measure_step_time(const PipelineConfig& config, double dt) {
  // TODO: feed 0 for the first 100 readings and 1 afterwards, 2000 in all. Return the seconds, (i - 100) * dt,
  // from the step to the first output of 0.5 or more; -1 if it never gets there.
  (void)config;
  (void)dt;
  return -2.0;
}

// How much of a single wild reading of size `spike` gets through: a steady zero with one reading in a hundred replaced
// by the spike, the largest output after the first 200 readings, as a share of the spike.
inline double measure_leak(const PipelineConfig& config, double spike) {
  // TODO: feed zero, except `spike` at every reading where i % 100 == 50, 2000 in all.
  // The largest |output| after the first 200 readings, divided by |spike|. Zero for a spike of zero.
  (void)config;
  (void)spike;
  return -1.0;
}

#endif  // LESSON_SOLUTION_HPP
