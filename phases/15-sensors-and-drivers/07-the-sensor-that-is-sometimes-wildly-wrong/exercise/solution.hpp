#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <vector>

// The middle value of n numbers, found by rearranging them. Given: nth_element puts
// the middle one where it belongs without sorting the rest. For an even count this is
// the upper of the two middle values, a stated choice, so that a filter that is still
// filling has one rule to follow.
inline double middle_of(double* values, std::size_t n) {
  double* middle = values + n / 2;
  std::nth_element(values, middle, values + n);
  return *middle;
}

// The median of the last `window` readings.
class MedianFilter {
 public:
  explicit MedianFilter(std::size_t window)
      : window_(window == 0 ? 1 : window), ring_(window_, 0.0), scratch_(window_, 0.0) {}

  double update(double x) {
    // TODO: put x in the ring at the head and move the head on, wrapping. Count the
    // readings until the window is full.
    //
    // Copy the readings that are valid into scratch_, because middle_of rearranges
    // what it is given and the ring has to stay in order of arrival. While the window
    // is filling the valid readings are the first count_ slots of the ring.
    //
    // The answer is middle_of(scratch_.data(), count_).
    (void)x;
    (void)head_;
    (void)count_;
    (void)ring_;
    (void)scratch_;
    return 0.0;
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
    // TODO: the same ring as the median filter.
    //
    // median     the middle of the valid readings, including x
    // mad        the middle of |reading - median| over the same readings
    // scale      1.4826 * mad, which makes it an estimate of a standard deviation for
    //            ordinary noise, but never less than min_scale_: a sensor that has read
    //            the same number for a whole window has a spread of exactly zero, and
    //            then every change at all is infinitely far from it
    //
    // If |x - median| is more than threshold_ * scale, count it in rejected_ and
    // return the median. Otherwise return x exactly as it came in.
    (void)x;
    (void)threshold_;
    (void)min_scale_;
    (void)deviations_;
    (void)rejected_;
    (void)head_;
    (void)count_;
    (void)ring_;
    (void)scratch_;
    return 0.0;
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

#endif  // LESSON_SOLUTION_HPP
