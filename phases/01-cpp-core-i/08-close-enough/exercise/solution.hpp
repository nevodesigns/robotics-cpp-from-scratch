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
// TODO 1: how far it is to the next representable double.
//
// std::nextafter(x, towards) gives the next double after x in the direction of
// towards. Ask for the next one above, and subtract. Work on the magnitude,
// std::fabs(value), so that negative values answer the same as positive ones.
//
// The test checks that the gap grows with magnitude, and that at 1e16 it is
// exactly 2.0, which is why adding 1.0 there does nothing.
inline double gap_above(double value) {
  (void)value;
  return 0.0;
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
// TODO 2: roughly how many representable doubles separate these two.
//
// The distance between them, divided by the gap at that magnitude. Use the
// smaller magnitude of the two for the gap, since that is where the doubles
// are closest together and it gives the larger, more cautious count.
//
// Two cases to answer before dividing: identical values are nought apart, and
// a gap of zero would be a division by zero.
inline double ulps_between(double a, double b) {
  (void)a;
  (void)b;
  return 0.0;
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
// TODO 3: close enough, at any magnitude and at zero.
//
// Take the absolute difference once. Then two chances to pass:
//
//   within the absolute floor                      -> equal
//   within relative times the larger magnitude     -> equal
//
// The order matters. Check the absolute floor first, because that is the one
// that has to answer for values near zero where there is no magnitude to be
// relative to.
//
// Use the larger of the two magnitudes, std::fmax(std::fabs(a), std::fabs(b)),
// so the answer does not depend on which argument came first.
inline bool nearly_equal(double a, double b, double relative, double absolute) {
  (void)a;
  (void)b;
  (void)relative;
  (void)absolute;
  return false;
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
// TODO 4: add them all up without losing the small ones.
//
// Keep two variables, a running total and whatever was lost on the way in.
// For each value:
//
//   adjusted = value - lost          put back what was dropped last time
//   next     = total + adjusted      the addition that may drop some of it
//   lost     = (next - total) - adjusted    what it dropped, recovered
//   total    = next
//
// Keep those parentheses exactly as written. They look like they cancel to
// zero and algebraically they do; the whole algorithm is that in floating
// point they do not.
//
// The test adds 1.0 to a total of 1e16 a hundred thousand times. Plain
// addition loses every single one.
inline double kahan_sum(rc::span<const double> values) {
  (void)values;
  return 0.0;
}

#endif  // LESSON_SOLUTION_HPP
