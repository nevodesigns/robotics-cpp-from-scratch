// rc/math/derivative.hpp
//
// A slope taken from a function and a rate taken from measurements, from lesson
// 06-05. They use the same arithmetic and they are not the same problem.
//
// Taking the slope of a function you can call is a floating point problem. The
// step has to be small enough that the curve is nearly straight over it and
// large enough that subtracting two nearby values leaves something, and those
// pull in opposite directions. Measured on sin(x)e^(0.3x) at x = 1.2:
//
//   step     forward     central   complex step
//   1e-06   4.518e-07   2.055e-12      2.578e-13
//   1e-08   1.388e-08   8.329e-09      2.220e-16
//   1e-12   4.432e-04   1.101e-04      1.110e-16
//   1e-16   9.202e-01   9.202e-01      2.220e-16
//
// At 1e-16 the forward and central differences are out by the whole of the
// derivative. Each method has a best step and it is where the theory says: the
// square root of the machine epsilon for a forward difference and its cube root
// for a central one.
//
// Taking a rate from measurements is not that problem at all. The interval is
// however often the sensor speaks, making it smaller makes the answer worse,
// and what decides the result is the noise: one millimetre of position noise at
// 500 Hz measured as 0.7049 m/s of velocity noise against 0.7071 predicted.

#ifndef RC_MATH_DERIVATIVE
#define RC_MATH_DERIVATIVE

#include <cmath>
#include <limits>

namespace rc {
namespace math {

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

}  // namespace math
}  // namespace rc

#endif  // RC_MATH_DERIVATIVE
