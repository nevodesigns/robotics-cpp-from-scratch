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
    const double calibrated = config_.calibration.apply(raw);
    const double clean = config_.use_hampel ? hampel_.update(calibrated) : calibrated;
    return smoother_.update(clean);
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
  Pipeline pipeline(config);
  detail::Noise noise;
  double in = 0.0;
  double out = 0.0;
  int counted = 0;
  for (int i = 0; i < 5000; ++i) {
    const double raw = noise.gaussian();
    const double y = pipeline.update(raw);
    if (i < 200) continue;
    in += raw * raw;
    out += y * y;
    ++counted;
  }
  if (counted == 0 || in <= 0.0) return 0.0;
  return std::sqrt(out / in);
}

// How far behind a ramp of one unit per second the output runs, in seconds, once it has settled:
// 1500 readings at `dt` seconds, the lag read from the last 100.
inline double measure_ramp_lag(const PipelineConfig& config, double dt) {
  Pipeline pipeline(config);
  double lag = 0.0;
  int counted = 0;
  for (int i = 0; i < 1500; ++i) {
    const double t = i * dt;
    const double y = pipeline.update(t);
    if (i >= 1400) {
      lag += t - y;
      ++counted;
    }
  }
  return counted ? lag / counted : 0.0;
}

// How long after a step from 0 to 1 the output first reaches 0.5, in seconds, with no noise. The step comes at
// reading 100. Negative if it has not got there after 2000 readings.
inline double measure_step_time(const PipelineConfig& config, double dt) {
  Pipeline pipeline(config);
  for (int i = 0; i < 2000; ++i) {
    const double y = pipeline.update(i < 100 ? 0.0 : 1.0);
    if (i >= 100 && y >= 0.5) return (i - 100) * dt;
  }
  return -1.0;
}

// How much of a single wild reading of size `spike` gets through: a steady zero with one reading in a hundred replaced
// by the spike, the largest output after the first 200 readings, as a share of the spike.
inline double measure_leak(const PipelineConfig& config, double spike) {
  Pipeline pipeline(config);
  double worst = 0.0;
  for (int i = 0; i < 2000; ++i) {
    const double raw = (i % 100 == 50) ? spike : 0.0;
    const double y = pipeline.update(raw);
    if (i >= 200 && std::fabs(y) > worst) worst = std::fabs(y);
  }
  return spike != 0.0 ? worst / std::fabs(spike) : 0.0;
}

#endif  // LESSON_SOLUTION_HPP
