// rc/core/statistics.hpp
//
// The running statistics from lesson 03-01, graduated.
//
// Count, mean, smallest and largest, kept as readings arrive rather than by
// storing them all, which is what lets it run for a week on a device with no
// room to store a week of readings.

#ifndef RC_CORE_STATISTICS
#define RC_CORE_STATISTICS

#include <algorithm>
#include <cmath>

namespace rc {
namespace core {

class RunningStatistics {
 public:
  RunningStatistics() = default;

  void add(double value) {
    // The first reading defines both ends of the range. Seeding them from zero
    // instead would report a lowest of 0.0 for a sensor that never goes near it.
    if (count_ == 0) {
      lowest_ = value;
      highest_ = value;
    } else {
      lowest_ = std::min(lowest_, value);
      highest_ = std::max(highest_, value);
    }

    ++count_;

    // Welford's method. Nothing here ever holds the sum of the readings, so
    // precision is not spent representing a large constant offset, and the
    // small variations survive.
    const double delta = value - mean_;
    mean_ += delta / count_;

    // Deliberately computed against the updated mean. Using the old mean twice
    // is the classic misremembering of this algorithm and gives a variance that
    // is subtly wrong rather than obviously so.
    const double delta_after = value - mean_;
    m2_ += delta * delta_after;
  }

  int count() const { return count_; }
  double mean() const { return mean_; }
  double lowest() const { return lowest_; }
  double highest() const { return highest_; }

  // The sample variance. One reading has no spread, and dividing by count - 1
  // would divide by zero, so both problems are answered by the same guard.
  double variance() const {
    if (count_ < 2) return 0.0;
    return m2_ / (count_ - 1);
  }

  double standard_deviation() const { return std::sqrt(variance()); }

  void reset() {
    count_ = 0;
    mean_ = 0.0;
    m2_ = 0.0;
    lowest_ = 0.0;
    highest_ = 0.0;
  }

 private:
  int count_ = 0;
  double mean_ = 0.0;
  double m2_ = 0.0;
  double lowest_ = 0.0;
  double highest_ = 0.0;
};

}  // namespace core
}  // namespace rc

#endif  // RC_CORE_STATISTICS
