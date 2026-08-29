#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <algorithm>
#include <numeric>
#include <optional>
#include <vector>

struct Reading {
  double celsius = 0.0;
  long at_ms = 0;
  bool valid = true;
};

// Each of these is one algorithm call. Reach for the named algorithm rather
// than writing the loop, and the bounds stop being yours to get wrong.

// The first reading above the limit, or nothing.
inline std::optional<Reading> first_above(const std::vector<Reading>& readings, double limit) {
  // TODO: std::find_if. Remember that not found is end(), and that
  // dereferencing end() is undefined behaviour, so compare before you read.
  (void)readings;
  (void)limit;
  return std::nullopt;
}

// How many readings are marked valid.
inline int count_valid(const std::vector<Reading>& readings) {
  // TODO: std::count_if
  (void)readings;
  return 0;
}

// The mean temperature, and 0.0 for an empty batch.
inline double mean_celsius(const std::vector<Reading>& readings) {
  // TODO: std::accumulate. Mind the starting value: it decides the type of
  // every addition, and an integer one truncates at each step.
  (void)readings;
  return 0.0;
}

// A new vector of the same length, converted to Fahrenheit.
inline std::vector<double> to_fahrenheit(const std::vector<Reading>& readings) {
  // TODO: std::transform
  (void)readings;
  return {};
}

// The comparator. It must be strictly less than: false when both arguments are
// the same reading. Writing <= here makes std::sort walk off the end of the
// range, which is the crash triaged in lesson 04-02.
inline bool earlier(const Reading& a, const Reading& b) {
  // TODO
  (void)a;
  (void)b;
  return false;
}

inline void sort_by_time(std::vector<Reading>& readings) {
  // TODO: std::sort, using earlier
  (void)readings;
}

// Removes every invalid reading. The vector must actually get shorter.
inline void drop_invalid(std::vector<Reading>& readings) {
  // TODO: remove_if shuffles the keepers to the front and returns the new end.
  // It cannot resize the container. The erase is what does that.
  (void)readings;
}

#endif  // LESSON_SOLUTION_HPP
