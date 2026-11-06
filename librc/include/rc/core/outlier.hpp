// rc/core/outlier.hpp
//
// A median and a Hampel filter, from lesson 15-07, graduated.
//
// rc/core/filters.hpp has a mean and a median of a whole array. These work one
// reading at a time, which is how a sensor delivers them, for a sensor that is
// sometimes wildly wrong.
//
// Numbers from that lesson. On a sine with noise of 0.05 and 211 of 20000 readings
// wrong by 50 to 100, the rms error was 7.681 raw, 2.519 through a mean of nine,
// 0.092 through a median of nine (0.025 once its four samples of delay are allowed
// for) and 0.053 through a Hampel filter, which leaves ordinary noise as it found it.
//
// A median of nine outvotes four bad readings in a row and passes the fifth: its
// breakdown point is half the window. And a real step is held back for four readings,
// because on its first reading it is indistinguishable from a spike.

#ifndef RC_CORE_OUTLIER_HPP
#define RC_CORE_OUTLIER_HPP

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <vector>

namespace rc {
namespace core {

// The middle value of n numbers, found by rearranging them: nth_element puts the
// middle one where it belongs without sorting the rest, which is all a median needs.
// For an even count this is the upper of the two middle values, which is a choice and
// is stated: a streaming filter that is still filling has to answer, and one rule
// beats an average that no reading ever took.
inline double middle_of(double* values, std::size_t n) {
  double* middle = values + n / 2;
  std::nth_element(values, middle, values + n);
  return *middle;
}

// The median of the last `window` readings.
//
// A mean is pulled by one bad reading in proportion to how bad it is: a spike of 100
// in a window of nine moves it by 11. A median ignores how bad it is and only counts
// which side of the middle it falls, so a spike moves it by nothing at all. That is
// why a sensor that now and then reports nonsense wants one.
//
// Until the window has filled it answers with the median of what it has.
class MedianFilter {
 public:
  explicit MedianFilter(std::size_t window)
      : window_(window == 0 ? 1 : window), ring_(window_, 0.0), scratch_(window_, 0.0) {}

  double update(double x) {
    ring_[head_] = x;
    head_ = (head_ + 1) % window_;
    if (count_ < window_) ++count_;

    // Copied out, because finding the middle rearranges what it is given and the
    // ring has to stay in order of arrival. While filling, the readings are the first
    // `count_` slots, since the head has not yet gone round.
    for (std::size_t i = 0; i < count_; ++i) scratch_[i] = ring_[i];
    return middle_of(scratch_.data(), count_);
  }

  std::size_t window() const { return window_; }

  // How late a steady signal comes out: the middle of a window is half a window back.
  std::size_t delay_samples() const { return (window_ - 1) / 2; }

 private:
  std::size_t window_;
  std::vector<double> ring_;
  std::vector<double> scratch_;
  std::size_t head_ = 0;
  std::size_t count_ = 0;
};

// Replaces a reading that is far from its neighbours, and passes every other one
// through untouched.
//
// "Far" is measured against the window's own spread, not a fixed number: the median
// absolute deviation, times 1.4826 so that it estimates a standard deviation for
// ordinary noise. A reading more than `threshold` of those from the window's median is
// replaced by the median.
//
// It does not smooth. A reading that is not an outlier comes out exactly as it went
// in, noise and all, so there is no lag on a signal that is behaving. The price is
// that it cannot tell a real step from a spike on the first reading it sees.
class HampelFilter {
 public:
  HampelFilter(std::size_t window, double threshold = 3.0, double min_scale = 0.0)
      : window_(window == 0 ? 1 : window),
        threshold_(threshold),
        min_scale_(min_scale),
        ring_(window_, 0.0),
        scratch_(window_, 0.0),
        deviations_(window_, 0.0) {}

  double update(double x) {
    ring_[head_] = x;
    head_ = (head_ + 1) % window_;
    if (count_ < window_) ++count_;

    for (std::size_t i = 0; i < count_; ++i) scratch_[i] = ring_[i];
    const double median = middle_of(scratch_.data(), count_);

    for (std::size_t i = 0; i < count_; ++i) deviations_[i] = std::fabs(ring_[i] - median);
    const double mad = middle_of(deviations_.data(), count_);

    // A floor on the spread. A sensor that has read the same number for a window has
    // a spread of exactly zero, and then every change at all, however small and
    // however real, is infinitely far from it. The floor is the least the sensor
    // can resolve, which a caller knows and the window cannot.
    const double scale = std::max(1.4826 * mad, min_scale_);

    if (std::fabs(x - median) > threshold_ * scale) {
      ++rejected_;
      return median;
    }
    return x;
  }

  std::size_t rejected() const { return rejected_; }

 private:
  std::size_t window_;
  double threshold_;
  double min_scale_;
  std::vector<double> ring_;
  std::vector<double> scratch_;
  std::vector<double> deviations_;
  std::size_t head_ = 0;
  std::size_t count_ = 0;
  std::size_t rejected_ = 0;
};

}  // namespace core
}  // namespace rc

#endif  // RC_CORE_OUTLIER_HPP
