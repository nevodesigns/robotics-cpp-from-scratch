// rc/core/approx.hpp
//
// Comparing numbers that are never exact, from lesson 01-08, graduated.
//
// Lesson 00-04 said never to compare two computed doubles for exact equality
// and to use a tolerance instead. This is the rest of that sentence, because
// choosing the tolerance is the hard half.
//
// Doubles are not spread evenly along the number line, and every tolerance
// decision is really a question about the gap between neighbours:
//
//   magnitude    gap to next    can it notice v + 1.0
//        1e00      2.220e-16    yes
//        1e06      1.164e-10    yes
//        1e12      1.221e-04    yes
//        1e16      2.000e+00    NO
//        1e17      1.600e+01    NO
//
// Which is why an absolute tolerance means two different things. The same
// 1e-9, read as an absolute distance:
//
//   magnitude    doubles it spans    adjacent pair within it
//        1e00           4.504e+06    yes, and four million others
//        1e06           8.590e+00    yes
//        1e09           8.389e-03    NO, and so no two distinct doubles are
//        1e12           8.192e-06    NO
//
// At magnitude 1 it is loose enough to cover four and a half million distinct
// values. At 1e9 it cannot reach the next representable double, so nothing but
// an identical value can ever satisfy it. Read relatively, the same 1e-9 spans
// between 4.5e6 and 8.8e6 doubles at every magnitude from 1 to 1e12, a factor
// of two across twelve orders of magnitude.
//
// A relative tolerance alone is not enough either, because near zero there is
// no magnitude to be relative to: 1e-18 and 2e-18 are a hundred percent apart,
// and nothing is ever close to zero. So nearly_equal takes both, and the
// absolute one exists for that case and no other.
//
// On accumulation. Stepping a clock by 0.1 and waiting for it to equal 10.0
// never arrives, at all, ever; waiting for it to reach 10.0 arrives after 101
// steps rather than 100, because a hundred additions of 0.1 land on
// 9.99999999999998046. And drift grows with the step count: 1 representable
// double adrift after ten steps, about 92000 after a million.
//
// On summation. Adding 1.0 to a running total of 1e16, a hundred thousand
// times, moved the total by exactly zero, because 1.0 is below the gap between
// doubles there. Sorting smallest first fixes it, and so does kahan_sum, both
// landing on the exact answer.
//
// A note on rc::test::nearly_equal, which takes one absolute tolerance. That
// is the right shape for a test, where the expected magnitude is written on
// the line next to it and a reader can check the number by eye. It is the
// wrong shape for a comparison in a running system, where the magnitude is
// whatever the robot is doing at the time.

#ifndef RC_CORE_APPROX_HPP
#define RC_CORE_APPROX_HPP

#include <cmath>
#include <limits>

#include <rc/core/compat.hpp>

namespace rc {
namespace core {

// The distance from a value to the next representable double above it.
inline double gap_above(double value) {
  const double magnitude = std::fabs(value);
  return std::nextafter(magnitude, std::numeric_limits<double>::infinity()) - magnitude;
}

// Roughly how many representable doubles lie between two values.
//
// This is the unit a tolerance should be read in. Counted by division rather
// than by reinterpreting the bits: the bit trick is exact, and this is close
// enough to compare against a threshold, which is all anyone does with it.
inline double ulps_between(double a, double b) {
  if (a == b) return 0.0;
  const double smaller = std::fmin(std::fabs(a), std::fabs(b));
  const double gap = gap_above(smaller);
  if (gap <= 0.0) return 0.0;
  return std::fabs(a - b) / gap;
}

// Are these two close enough to treat as the same number.
//
// The relative tolerance does the work and means the same thing at every
// magnitude. The absolute one is a floor for values near zero, where there is
// no magnitude to be relative to. Both, because either alone has a range it
// cannot answer for.
inline bool nearly_equal(double a, double b, double relative, double absolute) {
  const double difference = std::fabs(a - b);
  if (difference <= absolute) return true;
  return difference <= relative * std::fmax(std::fabs(a), std::fabs(b));
}

// Adding up a list without losing the small numbers.
//
// Plain addition discards whatever will not fit in the running total, and once
// the total is large enough the small values stop arriving at all. This keeps
// the lost part and feeds it back in.
inline double kahan_sum(rc::span<const double> values) {
  double total = 0.0;
  double lost = 0.0;
  for (double value : values) {
    const double adjusted = value - lost;
    const double next = total + adjusted;
    // Those parentheses are the algorithm. Algebraically they cancel; in
    // floating point they do not, and an optimiser permitted to reassociate
    // them computes zero and throws the correction away.
    lost = (next - total) - adjusted;
    total = next;
  }
  return total;
}

}  // namespace core
}  // namespace rc

#endif  // RC_CORE_APPROX_HPP
