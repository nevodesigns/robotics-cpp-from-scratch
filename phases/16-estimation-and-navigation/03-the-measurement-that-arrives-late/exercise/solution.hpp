#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cmath>

#include <rc/core/clock.hpp>
#include <rc/nav/filter.hpp>

using rc::nav::Estimate;

// An absolute measurement that describes a moment already past.
struct Delayed {
  double value = 0.0;
  double variance = 1.0;
  rc::core::Nanoseconds sampled_at = 0;
};

// TODO 1: move the measurement to the present.
//
// The value moves by motion_since, which is what the robot did between the
// moment the measurement describes and now. The variance grows by
// motion_variance_since, because the motion you added was itself measured.
//
// Two lines. The first is the whole of the fix; the second is what makes the
// filter's claim about its own accuracy mean something, and it costs a little
// accuracy in exchange, because a measurement trusted less is used less.
inline Estimate project_forward(const Delayed& measurement, double motion_since,
                                double motion_variance_since) {
  (void)motion_since;
  (void)motion_variance_since;
  return Estimate{measurement.value, measurement.variance};
}

// TODO 2: the residual, in units of what the filter expected.
//
// residual is the measurement minus the prediction. expected_deviation is the
// square root of the two variances added, which is how far apart the filter
// thinks those two numbers could reasonably be.
//
// normalised() is the ratio, and it is the only form worth looking at: ten
// centimetres is enormous from a laser and nothing from a first GPS fix.
// Guard against a zero or negative deviation rather than dividing by it.
struct Innovation {
  double residual = 0.0;
  double expected_deviation = 1.0;

  double normalised() const { return 0.0; }
};

inline Innovation innovation_of(const Estimate& prior, const Estimate& measurement) {
  (void)prior;
  (void)measurement;
  return Innovation{};
}

// TODO 3: a running check on whether the innovations match the filter's claims.
//
// add() takes an innovation and accumulates its normalised value. mean() and
// rms() report what has accumulated, and both return 0.0 before anything has.
//
// On a filter whose variances describe what is really happening, rms() sits
// around one. Well above one means the filter is more wrong than it believes.
class ConsistencyMonitor {
 public:
  void add(const Innovation& innovation) { (void)innovation; }

  int count() const { return count_; }
  double mean() const { return 0.0; }
  double rms() const { return 0.0; }

 private:
  double sum_ = 0.0;
  double sum_squared_ = 0.0;
  int count_ = 0;
};

#endif  // LESSON_SOLUTION_HPP
