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

// TODO 1: the step size that balances the two errors, scaled to x.
//
// Truncation error grows as the step and cancellation error falls as it, so
// their sum for a central difference is smallest around the cube root of
// std::numeric_limits<double>::epsilon(), which is about 6e-6.
//
// Multiply that by the larger of |x| and 1. Not by |x| alone, or the step goes
// to zero along with x; not by 1 alone, or a step of 1e-6 is asked to be small
// next to 1e12, where it is smaller than the gap between the doubles and adding
// it does nothing at all. The test in this lesson prints both failures.
inline double suggested_step(double x) {
  (void)x;
  return 1e-6;
}

// TODO 2: the slope of f at x, by central difference.
//
//     (f(x + h) - f(x - h)) / ((x + h) - (x - h))
//
// Both sides, not one. A forward difference is one call cheaper and about six
// thousand times worse at its own best step, because the errors on the two
// sides cancel rather than add.
//
// Divide by the span the arithmetic actually produced, not by 2h. Adding h to x
// and subtracting it again does not in general give exactly 2h, and using the
// number you meant instead of the number you got throws away a digit for free.
//
// Return 0.0 if the two points come out equal, which means the step vanished.
template <class F>
double derivative(F f, double x, double h) {
  (void)f;
  (void)x;
  (void)h;
  return 0.0;
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

// TODO 3: what noise that rate will have, given the noise on each measurement.
//
// Two independent samples are subtracted, so their noise adds in quadrature to
// sigma times the square root of two, and the whole thing is then divided by
// the interval. Return 0.0 for an interval of zero rather than dividing by it.
//
// The consequence is the useful part, and the test measures it: widening the
// interval divides this by the number of samples it spans, because the noise in
// the numerator does not grow while the denominator does. Filtering the noisy
// rate afterwards divides it by the square root of that, for the same delay.
inline double rate_noise(double sample_noise, double interval) {
  (void)sample_noise;
  (void)interval;
  return 0.0;
}

#endif  // LESSON_SOLUTION_HPP
