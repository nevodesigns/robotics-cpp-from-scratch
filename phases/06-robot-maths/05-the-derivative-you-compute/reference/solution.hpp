#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cmath>
#include <limits>

// Two different problems that look like one.
//
// Taking the slope of a function you can call is a floating point problem: the
// step has to be small enough that the curve is nearly straight over it and
// large enough that subtracting two nearby values leaves something. Those pull
// in opposite directions and the answer is a compromise nobody can improve on.
//
// Taking a rate from measurements is not that problem at all. There the step is
// however often the sensor speaks, and what decides the answer is the noise:
// one millimetre of position noise at 500 Hz is 0.7 metres per second of
// velocity noise, measured, which is the same number the derivative term of a
// controller sees.

// The step size that balances the two errors, scaled to the value it is taken
// at.
//
// Truncation error grows as the step and cancellation error falls as the step,
// so for a central difference the sum is smallest around the cube root of the
// machine epsilon, which is about 6e-6 for a double. Measured on one function,
// the best step was 1e-6 and the error there was 2.055e-12.
//
// Scaled to x, because a step is only small relative to something. A fixed 1e-6
// at x = 1e10 is smaller than the gap between representable doubles there, and
// at x = 1e12 the addition does nothing at all: x + h == x, and the derivative
// comes out as exactly zero.
inline double suggested_step(double x) {
  const double scale = std::fabs(x) > 1.0 ? std::fabs(x) : 1.0;
  return std::cbrt(std::numeric_limits<double>::epsilon()) * scale;
}

// The slope of f at x, by central difference.
//
// Both sides, not one. A forward difference costs one extra call less and is
// about six thousand times worse: its error at its own best step was 1.388e-8
// against 2.055e-12 for this one, because the errors on the two sides cancel
// rather than add.
//
// The step is read back from the arithmetic rather than assumed. Adding h to x
// and subtracting it again does not in general give 2h, and dividing by the
// number you meant instead of the number you got throws away a digit for free.
template <class F>
double derivative(F f, double x, double h) {
  const double above = x + h;
  const double below = x - h;
  const double span = above - below;
  if (span == 0.0) return 0.0;
  return (f(above) - f(below)) / span;
}

template <class F>
double derivative(F f, double x) {
  return derivative(f, x, suggested_step(x));
}

// A rate from two measurements an interval apart.
//
// The same arithmetic and an entirely different problem: here the interval is
// not yours to choose freely, and making it smaller makes the answer worse.
inline double rate_over(double earlier, double later, double interval) {
  if (interval == 0.0) return 0.0;
  return (later - earlier) / interval;
}

// What noise that rate will have, given the noise on each measurement.
//
// Two independent samples are subtracted, so their noise adds in quadrature to
// sigma times the square root of two, and then the whole thing is divided by
// the interval. Measured at 1 mm and 2 ms: 0.7065 against 0.7071 predicted.
//
// The consequence is the useful part. Widening the interval divides this by the
// number of samples it spans, because the noise in the numerator does not grow
// while the denominator does. Filtering the noisy rate afterwards divides it by
// the square root of that, for the same delay. Widen the interval.
inline double rate_noise(double sample_noise, double interval) {
  if (interval == 0.0) return 0.0;
  return std::fabs(sample_noise) * std::sqrt(2.0) / std::fabs(interval);
}

#endif  // LESSON_SOLUTION_HPP
