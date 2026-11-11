// rc/sim/sensor_model.hpp
//
// A simulated sensor that lies like the real one, from lesson 18-04, graduated.
//
// A bias, noise, a range, a resolution, a delay and dropouts, applied in the order a real sensor applies them (truth plus bias plus noise, held to its range, rounded to its resolution, delayed, then possibly lost), from a seed. The random numbers
// are drawn on every reading whatever the options, so switching one effect on does not change another. A lost reading is marked and carries the last good value.
//
// A bias of 0.3 on 2.0 read 2.3000 with no spread; noise of 0.05 read mean 1.9997 and spread 0.0502. A resolution of 0.1 gave an rms error of 0.02885 against 0.02887 for step over root twelve. A delay of 3 read 0 0 0 0 1 2 3 4 on a ramp. A dropout chance of 0.1
// lost 2052 of 20000 (0.103). A proportional controller overshot by 0.1945 with a perfect sensor and 0.6543 with one 60 ms old, and was still swinging by 0.0325 in the last half second against 0.0000.

#ifndef RC_SIM_SENSOR_MODEL_HPP
#define RC_SIM_SENSOR_MODEL_HPP

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <deque>

namespace rc {
namespace sim {

// What is wrong with the sensor. All zero (and the range open) is a perfect sensor.
struct SensorOptions {
  double bias = 0.0;                 // added to every reading
  double noise = 0.0;                // standard deviation of the noise added to every reading
  double low = -1e300;               // the reading cannot go below this, or above `high`: the range of the sensor
  double high = 1e300;
  double step = 0.0;                 // readings come in multiples of this (the resolution), zero for none
  int delay = 0;                     // readings are this many samples old
  double dropout = 0.0;              // the chance a reading does not arrive at all
  std::uint32_t seed = 1;
};

struct Reading {
  double value = 0.0;
  bool valid = false;                // false when the reading was lost: the value is then the last good one
};

// A sensor model. Every reading takes the truth through the same steps, in this order, which is the order they happen in a real sensor:
//   truth + bias + noise, held to its range, rounded to its resolution, then delayed, and finally the whole reading may be lost.
class SensorModel {
 public:
  explicit SensorModel(const SensorOptions& options) : options_(options), state_(options.seed == 0 ? 1u : options.seed) {}

  Reading read(double truth) {
    // The random numbers are drawn on every call whatever the options, so that changing one option does not change what the others do.
    const double n = gaussian();
    const double lost = uniform();

    double v = truth + options_.bias + options_.noise * n;
    if (v < options_.low) v = options_.low;
    if (v > options_.high) v = options_.high;
    if (options_.step > 0.0) v = std::round(v / options_.step) * options_.step;

    history_.push_back(v);
    const std::size_t keep = static_cast<std::size_t>(options_.delay) + 1;
    while (history_.size() > keep) history_.pop_front();
    const double delayed = history_.front();

    Reading r;
    if (lost < options_.dropout) {
      ++dropped_;
      r.value = last_good_;
      r.valid = false;
      return r;
    }
    last_good_ = delayed;
    r.value = delayed;
    r.valid = true;
    return r;
  }

  int dropped() const { return dropped_; }

 private:
  std::uint32_t next() {
    state_ ^= state_ << 13;
    state_ ^= state_ >> 17;
    state_ ^= state_ << 5;
    return state_;
  }
  double uniform() { return static_cast<double>(next() >> 8) / 16777216.0; }
  double gaussian() {
    double sum = 0.0;
    for (int i = 0; i < 12; ++i) sum += uniform();
    return sum - 6.0;
  }

  SensorOptions options_;
  std::uint32_t state_ = 1;
  std::deque<double> history_;
  double last_good_ = 0.0;
  int dropped_ = 0;
};

}  // namespace sim
}  // namespace rc

#endif  // RC_SIM_SENSOR_MODEL_HPP
