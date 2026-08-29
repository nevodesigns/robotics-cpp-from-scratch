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

inline std::optional<Reading> first_above(const std::vector<Reading>& readings, double limit) {
  const auto found = std::find_if(readings.begin(), readings.end(),
                                  [limit](const Reading& r) { return r.celsius > limit; });

  // Comparing against end() is compulsory. Dereferencing it is undefined
  // behaviour, and it is the same mistake as dereferencing a null pointer.
  if (found == readings.end()) return std::nullopt;
  return *found;
}

inline int count_valid(const std::vector<Reading>& readings) {
  return static_cast<int>(std::count_if(readings.begin(), readings.end(),
                                        [](const Reading& r) { return r.valid; }));
}

inline double mean_celsius(const std::vector<Reading>& readings) {
  if (readings.empty()) return 0.0;

  // The starting value decides the type of every addition. Writing 0 here
  // instead of 0.0 would add into an int and truncate at each step, which the
  // compiler accepts without a word.
  const double total = std::accumulate(
      readings.begin(), readings.end(), 0.0,
      [](double running, const Reading& r) { return running + r.celsius; });

  return total / static_cast<double>(readings.size());
}

inline std::vector<double> to_fahrenheit(const std::vector<Reading>& readings) {
  std::vector<double> out;
  out.reserve(readings.size());
  std::transform(readings.begin(), readings.end(), std::back_inserter(out),
                 [](const Reading& r) { return r.celsius * 9.0 / 5.0 + 32.0; });
  return out;
}

// Strictly less than. With <= an element would compare as before itself, and
// std::sort would walk off the end of the range hunting a pivot that cannot
// exist. That is the crash triaged in lesson 04-02.
inline bool earlier(const Reading& a, const Reading& b) { return a.at_ms < b.at_ms; }

inline void sort_by_time(std::vector<Reading>& readings) {
  std::sort(readings.begin(), readings.end(), earlier);
}

inline void drop_invalid(std::vector<Reading>& readings) {
  // remove_if only has iterators, and iterators cannot resize a container, so
  // it shuffles the keepers to the front and hands back the new end. The erase
  // is what actually shortens the vector. Calling remove_if alone changes
  // nothing about the size and reports no error at all.
  readings.erase(std::remove_if(readings.begin(), readings.end(),
                                [](const Reading& r) { return !r.valid; }),
                 readings.end());
}

#endif  // LESSON_SOLUTION_HPP
