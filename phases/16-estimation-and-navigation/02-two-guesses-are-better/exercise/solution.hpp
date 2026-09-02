#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cmath>

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
  // TODO: the prior's share of the total uncertainty.
  //
  // A prior you are sure of and a measurement you are not should give a small
  // number, so the estimate barely moves. The other way round should give a
  // number near one, so the measurement wins.
  //
  // Two estimates that are both certain carry no information about which to
  // prefer, and dividing by their total is a division by zero.
  (void)prior;
  (void)measurement;
  return 0.0;
}

// Two independent estimates of the same quantity, combined into one.
//
// The result is always more certain than either input, which is the part worth
// sitting with: combining a good estimate with a poor one still leaves you
// better off than the good one alone, so a bad sensor is worth having as long
// as you are honest about how bad it is.
inline Estimate fuse(const Estimate& a, const Estimate& b) {
  // TODO: move from a toward b by the gain, and reduce the uncertainty.
  //
  // The result must be more certain than *either* input, always. If your
  // version can produce a variance larger than one of the things it combined,
  // it is not combining them.
  (void)a;
  (void)b;
  return Estimate{};
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
    // TODO: move, and become less certain.
    //
    // The variance must grow here. A filter that predicts without adding
    // uncertainty becomes more confident every step it takes on no evidence at
    // all, and eventually refuses to listen to anything.
    (void)motion;
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

#endif  // LESSON_SOLUTION_HPP
