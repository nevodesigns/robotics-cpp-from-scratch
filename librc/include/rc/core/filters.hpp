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

}  // namespace core
}  // namespace rc

#endif  // RC_CORE_FILTERS
