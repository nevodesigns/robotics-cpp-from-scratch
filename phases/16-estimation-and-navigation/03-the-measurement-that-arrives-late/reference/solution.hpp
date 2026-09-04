#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cmath>

#include <rc/core/clock.hpp>
#include <rc/nav/filter.hpp>

using rc::nav::Estimate;

// An absolute measurement that describes a moment already past.
//
// A camera fix, a beacon range, a scan match. All of them take time to produce,
// and by the time the number exists the robot has moved. Odometry arrives every
// 2 ms and a fix every 100 ms, eighty of which were spent computing it.
struct Delayed {
  double value = 0.0;
  double variance = 1.0;
  rc::core::Nanoseconds sampled_at = 0;
};

// The same measurement, moved to the present.
//
// motion_since is what the robot did between the moment the measurement
// describes and now, which the caller has because it is the odometry it has
// already used. motion_variance_since is how uncertain that motion was.
//
// Both halves are needed and only the first changes the answer much. Carrying
// the value forward is the whole of the fix; inflating the variance is what
// makes the filter's own claim about itself honest, and it costs a little
// accuracy, because a measurement trusted less is a measurement used less.
inline Estimate project_forward(const Delayed& measurement, double motion_since,
                                double motion_variance_since) {
  Estimate moved;
  moved.value = measurement.value + motion_since;
  moved.variance = measurement.variance + motion_variance_since;
  return moved;
}

// What the filter expected against what it got, in units of what it expected.
//
// The residual on its own means nothing: ten centimetres is enormous from a
// laser and nothing from a first GPS fix. Divided by the deviation the filter
// itself predicted, it is a number that should sit around one, whatever the
// sensor.
struct Innovation {
  double residual = 0.0;
  double expected_deviation = 1.0;

  double normalised() const {
    if (!(expected_deviation > 0.0)) return 0.0;
    return residual / expected_deviation;
  }
};

inline Innovation innovation_of(const Estimate& prior, const Estimate& measurement) {
  Innovation result;
  result.residual = measurement.value - prior.value;
  result.expected_deviation = std::sqrt(prior.variance + measurement.variance);
  return result;
}

// A running check on whether the innovations look like the filter's own claims.
//
// This is the only instrument in this lesson that looks at the filter from the
// outside, and its limits are as important as its use: it compares the
// measurement against the prediction, and both of those can be wrong together.
// It sees an error that changes and is blind to one that does not.
class ConsistencyMonitor {
 public:
  void add(const Innovation& innovation) {
    const double n = innovation.normalised();
    sum_ += n;
    sum_squared_ += n * n;
    ++count_;
  }

  int count() const { return count_; }

  double mean() const { return count_ == 0 ? 0.0 : sum_ / count_; }

  // Around one on a filter whose variances describe what is really happening.
  // Well above one means the filter is more wrong than it believes.
  double rms() const {
    return count_ == 0 ? 0.0 : std::sqrt(sum_squared_ / static_cast<double>(count_));
  }

 private:
  double sum_ = 0.0;
  double sum_squared_ = 0.0;
  int count_ = 0;
};

#endif  // LESSON_SOLUTION_HPP
