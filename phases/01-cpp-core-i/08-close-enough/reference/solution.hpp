#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cmath>
#include <limits>

#include <rc/core/compat.hpp>

// Lesson 00-04 said never to compare two computed doubles for exact equality,
// and to ask whether they are close enough instead. This lesson is about the
// second half of that sentence, which is harder than it sounds: close enough
// to what, measured how, and the same answer at every magnitude.

// The distance from a value to the next representable double above it.
//
// Doubles are not spread evenly along the number line. Near 1.0 they are about
// 2.2e-16 apart, and near 1e16 they are 2.0 apart, which is why adding 1.0 to
// 1e16 changes nothing at all. Every tolerance decision is really a question
// about this number.
inline double gap_above(double value) {
  const double magnitude = std::fabs(value);
  return std::nextafter(magnitude, std::numeric_limits<double>::infinity()) - magnitude;
}

// Roughly how many representable doubles lie between two values.
//
// This is the unit a tolerance should be read in. A tolerance of 1e-9 sounds
// small and sounds like one thing, and at magnitude 1.0 it spans about four and
// a half million distinct doubles while at magnitude 1e9 it spans less than
// one.
//
// Counted by division rather than by reinterpreting the bits. The bit trick is
// exact and this is close enough to compare against a threshold, which is all
// anybody does with it.
inline double ulps_between(double a, double b) {
  if (a == b) return 0.0;
  const double smaller = std::fmin(std::fabs(a), std::fabs(b));
  const double gap = gap_above(smaller);
  if (gap <= 0.0) return 0.0;
  return std::fabs(a - b) / gap;
}

// Are these two close enough to treat as the same number.
//
// Two tolerances, because one is never enough.
//
// The relative one is the one that does the work. It scales with the size of
// the numbers, so it means the same thing at every magnitude: measured across
// magnitudes from 1 to 1e12, a relative tolerance of 1e-9 spans about eight
// million representable doubles at every one of them, while an absolute
// tolerance of 1e-9 goes from four and a half million down to less than one.
//
// The absolute one is there for zero. A relative tolerance compares against the
// size of the values, and near zero there is no size to compare against, so
// 1e-18 and 2e-18 are a hundred percent apart and nothing is ever close to
// zero. The absolute floor answers that, and only that.
inline bool nearly_equal(double a, double b, double relative, double absolute) {
  const double difference = std::fabs(a - b);
  if (difference <= absolute) return true;
  return difference <= relative * std::fmax(std::fabs(a), std::fabs(b));
}

// Adding up a list without losing the small numbers.
//
// Plain addition throws away whatever will not fit in the running total, and
// once the total is large enough the small values stop arriving entirely:
// adding 1.0 to a total of 1e16, a hundred thousand times over, measured a
// change of exactly zero.
//
// This keeps the lost part in a second variable and feeds it back in. Four
// lines, and the same hundred thousand additions land exactly.
inline double kahan_sum(rc::span<const double> values) {
  double total = 0.0;
  double lost = 0.0;
  for (double value : values) {
    const double adjusted = value - lost;
    const double next = total + adjusted;
    // What did not fit. Written this way on purpose: the parentheses are the
    // algorithm, and an optimiser allowed to reassociate them computes zero.
    lost = (next - total) - adjusted;
    total = next;
  }
  return total;
}

#endif  // LESSON_SOLUTION_HPP
