#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cmath>

// Running statistics over readings arriving one at a time.
//
// The invariants this class must never break:
//   count is never negative
//   with no readings, every answer is 0.0 rather than leftover memory
//   with readings, lowest <= mean <= highest
//   variance is never negative
class RunningStatistics {
 public:
  RunningStatistics() = default;

  // Folds one reading in. Use Welford's method for the mean and the variance
  // accumulator, described in docs/en.md, rather than keeping a running total.
  void add(double value) {
    // TODO
    (void)value;
  }

  // TODO: mark every one of these const. A function that does not change the
  // object must say so, or the object cannot be used through a const reference.
  int count() { return count_; }

  double mean() { return mean_; }

  double lowest() { return lowest_; }

  double highest() { return highest_; }

  // The sample variance, dividing by count - 1. Zero when fewer than two
  // readings have been seen, because a single reading has no spread.
  double variance() {
    // TODO
    return 0.0;
  }

  double standard_deviation() {
    // TODO
    return 0.0;
  }

  // Back to the starting state.
  void reset() {
    // TODO
  }

 private:
  int count_ = 0;
  double mean_ = 0.0;
  double m2_ = 0.0;       // the accumulator Welford's method needs for variance
  double lowest_ = 0.0;
  double highest_ = 0.0;
};

#endif  // LESSON_SOLUTION_HPP
