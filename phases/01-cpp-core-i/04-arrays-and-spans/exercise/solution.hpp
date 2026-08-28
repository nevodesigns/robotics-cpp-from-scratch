#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <algorithm>
#include <rc/core/compat.hpp>
#include <vector>

// The average of the samples. Returns 0.0 for an empty span.
inline double mean(rc::span<const double> samples) {
  // TODO
  (void)samples;
  return 0.0;
}

// The middle value once sorted. With an even count, the average of the two
// middle values. Returns 0.0 for an empty span.
//
// The span is a read only view of the caller's data, so copy before sorting.
inline double median(rc::span<const double> samples) {
  // TODO
  (void)samples;
  return 0.0;
}

// One output per input. Each entry is the mean of that reading and up to
// window minus one readings before it. Returns an empty vector when window
// is less than one.
inline std::vector<double> moving_average(rc::span<const double> samples, int window) {
  // TODO
  (void)samples;
  (void)window;
  return {};
}

#endif  // LESSON_SOLUTION_HPP
