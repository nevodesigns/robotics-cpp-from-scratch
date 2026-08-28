#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <algorithm>
#include <rc/core/compat.hpp>
#include <vector>

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

#endif  // LESSON_SOLUTION_HPP
