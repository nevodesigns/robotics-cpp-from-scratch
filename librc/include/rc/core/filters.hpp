// rc/core/filters.hpp
//
// The filters from lesson 01-04, graduated.
//
// A mean is pulled by one bad reading and a median is not, which is the whole
// reason both are here: a sensor that occasionally reports nonsense needs the
// second, and a sensor with steady noise is better served by the first.

#ifndef RC_CORE_FILTERS
#define RC_CORE_FILTERS

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <vector>
#include <rc/core/compat.hpp>

namespace rc {
namespace core {

inline double mean(rc::span<const double> samples) {
  if (samples.empty()) return 0.0;

  double sum = 0.0;
  for (const double value : samples) sum += value;

  // samples.size() is unsigned, and dividing a double by it is fine, but the
  // cast makes the intent explicit and silences a warning on some compilers.
  return sum / static_cast<double>(samples.size());
}

inline double median(rc::span<const double> samples) {
  if (samples.empty()) return 0.0;

  // The span is a view of somebody else's data. Sorting through it would
  // reorder the caller's array behind their back, so copy first.
  std::vector<double> sorted(samples.begin(), samples.end());
  std::sort(sorted.begin(), sorted.end());

  const std::size_t middle = sorted.size() / 2;
  if (sorted.size() % 2 == 1) return sorted[middle];
  return (sorted[middle - 1] + sorted[middle]) / 2.0;
}

inline std::vector<double> moving_average(rc::span<const double> samples, int window) {
  if (window < 1) return {};

  std::vector<double> out;
  out.reserve(samples.size());

  for (std::size_t i = 0; i < samples.size(); ++i) {
    // The window is short at the start of the stream, because there is nothing
    // before the first reading. Averaging over what exists is more honest than
    // padding with zeros, which would drag the early output towards zero.
    const std::size_t span_size = std::min<std::size_t>(window, i + 1);
    const std::size_t first = i + 1 - span_size;
    out.push_back(mean(samples.subspan(first, span_size)));
  }
  return out;
}

// ---------------------------------------------------------------------------
// The running filter, from lesson 15-01.
//
// The functions above take a whole array. This is what a sensor needs: readings
// arrive one at a time, for ever, and the filter answers after each without
// storing a week of history.
//
// Choosing the window is a trade with two measured sides. Uncorrelated noise
// falls as one over the square root of it, and the delay it adds is the average
// age of what is in it, half the window. Both were confirmed against the theory
// in that lesson.
//
// And the delay is not free downstream. Measured with a controller steering by
// a filtered measurement: no filter at all never settles, because the
// derivative term differentiates the noise; a window of 64 settles in 1.18
// seconds with 0.3 percent overshoot; a window of 256, whose lag is a fifth of
// the settling time, overshoots by 103 percent and never settles at all.
// ---------------------------------------------------------------------------

// A moving average over the last N readings, kept as they arrive.
//
// Lesson 01-04 built the version that takes a whole array. This is the one a
// sensor actually needs: readings arrive one at a time, for ever, and the
// filter has to answer after each without storing a week of history.
//
// A ring of N doubles, no allocation after construction, no growth however long
// it runs. The sum is recomputed over the window rather than carried, which is
// N additions instead of two. Carried is what a fast loop should do, and its
// rounding drift over twenty million samples is 7.5e-10, which is nothing; the
// reason to write it this way here is that it cannot be wrong, and lesson 07-03
// is the place to reach for when a measurement says the additions matter.
class MovingAverage {
 public:
  explicit MovingAverage(int window)
      : window_(window < 1 ? 1 : static_cast<std::size_t>(window)) {
    samples_.assign(window_, 0.0);
  }

  double update(double reading) {
    samples_[next_] = reading;
    next_ = (next_ + 1) % window_;
    if (filled_ < window_) ++filled_;

    // Divided by how many readings there actually are, not by the window.
    //
    // Dividing by the window before it is full is the warm up transient: the
    // filter reports a fraction of the truth and climbs to it over N readings,
    // so a robot switched on next to a wall believes the wall is far away and
    // drives at it.
    double total = 0.0;
    for (std::size_t i = 0; i < filled_; ++i) total += samples_[i];
    return total / static_cast<double>(filled_);
  }

  // How much of the input noise survives, for uncorrelated noise: one over the
  // square root of the window. Measured in this lesson, and it is what says how
  // long a window has to be to be worth its lag.
  static double noise_gain(int window) {
    const double n = window < 1 ? 1.0 : static_cast<double>(window);
    return 1.0 / std::sqrt(n);
  }

  // And what it costs: the average age of the readings in the window, which is
  // the delay the loop downstream is going to feel.
  static double lag_samples(int window) {
    const double n = window < 1 ? 1.0 : static_cast<double>(window);
    return (n - 1.0) / 2.0;
  }

  int window() const { return static_cast<int>(window_); }
  bool warm() const { return filled_ == window_; }

 private:
  std::size_t window_ = 1;
  std::size_t next_ = 0;
  std::size_t filled_ = 0;
  std::vector<double> samples_;
};

}  // namespace core
}  // namespace rc

#endif  // RC_CORE_FILTERS
