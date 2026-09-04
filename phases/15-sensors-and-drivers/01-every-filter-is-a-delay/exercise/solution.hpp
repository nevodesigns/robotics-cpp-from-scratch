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
    // TODO: keep this reading, drop the one that has fallen out of the window,
    // and answer with the average of what is left.
    //
    // The oldest has to leave. A filter that never drops anything is a running
    // mean over all of history, which converges on the average of the whole run
    // and stops responding to the sensor.
    //
    // Divide by how many readings there actually are, not by the window. Before
    // the window is full those are different numbers, and dividing by the
    // window reports a fraction of the truth and climbs to it over N readings.
    // A robot switched on next to a wall then believes the wall is far away.
    (void)reading;
    return 0.0;
  }

  // How much of the input noise survives, for uncorrelated noise: one over the
  // square root of the window. Measured in this lesson, and it is what says how
  // long a window has to be to be worth its lag.
  static double noise_gain(int window) {
    // TODO: how much uncorrelated noise survives a window of this size.
    (void)window;
    return 1.0;
  }

  // And what it costs: the average age of the readings in the window, which is
  // the delay the loop downstream is going to feel.
  static double lag_samples(int window) {
    // TODO: the average age of the readings in the window, in samples, which is
    // the delay everything downstream is going to feel.
    (void)window;
    return 0.0;
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
