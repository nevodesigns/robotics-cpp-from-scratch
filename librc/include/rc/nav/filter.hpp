// rc/nav/filter.hpp
//
// The one dimensional filter from lesson 16-02, graduated, and the shape every
// larger one has.
//
// Two independent estimates of the same quantity combine into one that is more
// certain than either, always, which is why a poor second sensor is worth
// having as long as you are honest about how poor it is.
//
// Measured over two thousand steps of a drifting odometer and a noisy absolute
// sensor: dead reckoning alone 0.058 metres, the sensor alone 0.098, the two
// combined 0.024. Better than the better of them by a factor of two and a half.
//
// What the filter believes about its inputs matters about as much as the inputs
// themselves. Believing the odometry ten times better than it is costs 0.048,
// and ten times worse costs 0.053, each roughly double the honest figure. And
// those last two are the same mistake: only the ratio of the two beliefs sets
// the gain, so believing the odometry worse and believing the sensor better are
// the same statement about which to trust.

#ifndef RC_NAV_FILTER_HPP
#define RC_NAV_FILTER_HPP

#include <cmath>

namespace rc {
namespace nav {

// A number, and how sure of it you are.
//
// The variance is the square of the typical error, not the error itself, and
// keeping them apart is worth being pedantic about: a sensor described as
// "accurate to five centimetres" has a standard deviation of 0.05 and a
// variance of 0.0025, and using one where the other belongs is wrong by a
// factor of twenty here.
struct Estimate {
  double value = 0.0;
  double variance = 1.0;
};

inline double deviation(const Estimate& e) { return std::sqrt(e.variance); }

// How far to move from `prior` toward `measurement`, between nothing and all of
// the way.
//
// It is the prior's share of the total uncertainty. A prior you are sure of and
// a measurement you are not gives a small number and the estimate barely moves;
// the other way round gives a number near one and the measurement wins.
inline double gain_toward(const Estimate& prior, const Estimate& measurement) {
  const double total = prior.variance + measurement.variance;

  // Two estimates that are both certain carry no information about which to
  // prefer. Staying put is the only answer that does not divide by zero.
  if (total <= 0.0) return 0.0;
  return prior.variance / total;
}

// Two independent estimates of the same quantity, combined into one.
//
// The result is always more certain than either input, which is the part worth
// sitting with: combining a good estimate with a poor one still leaves you
// better off than the good one alone, so a bad sensor is worth having as long
// as you are honest about how bad it is.
inline Estimate fuse(const Estimate& a, const Estimate& b) {
  const double k = gain_toward(a, b);

  Estimate combined;
  combined.value = a.value + k * (b.value - a.value);
  combined.variance = (1.0 - k) * a.variance;
  return combined;
}

// One quantity, tracked through motion it is told about and measurements it is
// given.
class Filter1D {
 public:
  // motion_variance is how much uncertainty one step of motion adds. It is the
  // filter's opinion of the odometry from lesson 16-01, and getting it wrong
  // costs about as much as having worse odometry.
  Filter1D(const Estimate& start, double motion_variance)
      : estimate_(start), motion_variance_(motion_variance) {}

  // Move by what the odometer reported, and become less certain.
  //
  // The variance must grow here. A filter that predicts without adding
  // uncertainty becomes more confident every step it takes on no evidence at
  // all, and eventually refuses to listen to anything.
  void predict(double motion) {
    estimate_.value += motion;
    estimate_.variance += motion_variance_;
  }

  void correct(double measurement, double measurement_variance) {
    estimate_ = fuse(estimate_, Estimate{measurement, measurement_variance});
  }

  const Estimate& estimate() const { return estimate_; }
  double motion_variance() const { return motion_variance_; }

 private:
  Estimate estimate_;
  double motion_variance_ = 0.0;
};

}  // namespace nav
}  // namespace rc

#endif  // RC_NAV_FILTER_HPP
