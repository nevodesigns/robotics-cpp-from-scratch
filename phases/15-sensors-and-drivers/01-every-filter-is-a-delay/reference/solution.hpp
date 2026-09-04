#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cmath>
#include <cstddef>
#include <vector>

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

#endif  // LESSON_SOLUTION_HPP
