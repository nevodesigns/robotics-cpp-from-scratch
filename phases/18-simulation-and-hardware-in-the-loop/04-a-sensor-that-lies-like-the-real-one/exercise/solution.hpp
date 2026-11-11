#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <deque>

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
    // TODO: First draw the random numbers, ALWAYS, whatever the options: n = gaussian() and lost = uniform(), so that switching one option on does not change what another does.
    // v = truth + bias + noise * n; hold v to low..high; if step is above zero round it to a multiple of step: round(v / step) * step.
    // Delay: push v on the back of history_, keep only the last delay + 1 values (pop from the front), and the delayed reading is history_.front().
    // Then if lost < dropout: add one to dropped_ and return {last_good_, false}. Otherwise last_good_ = the delayed value and return {delayed, true}.
    (void)truth;
    (void)options_;
    (void)history_;
    (void)last_good_;
    (void)dropped_;
    return Reading();
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

#endif  // LESSON_SOLUTION_HPP
