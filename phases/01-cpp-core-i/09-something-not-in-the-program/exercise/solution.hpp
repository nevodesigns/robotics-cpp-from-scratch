#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <climits>

// Lesson 01-07 was about unsigned arithmetic, which wraps. Wrapping is defined
// behaviour: the answer is wrong and it is the same wrong answer on every
// machine and every compiler, which is why it is findable.
//
// Signed overflow is not that. It is undefined, which does not mean "the
// result is unpredictable". It means the standard places no requirement on the
// program at all, and the compiler is therefore entitled to assume it never
// happens and to optimise on that assumption.
//
// Which is why there is no checking after the fact. By the time a + b has
// overflowed there is no answer to inspect, and a test for a negative result
// is a test the compiler is allowed to delete. The question has to be asked
// first, and these four functions ask it.
//
// Every one of them must reach its answer without performing the operation it
// is asking about. That is the whole constraint, and it is what makes them
// look the way they do.

// Would a + b overflow.
//
// Rearranged so the arithmetic stays in range: instead of computing a + b and
// looking at it, compare a against the largest value that can still take b.
// TODO 1: would a + b overflow.
//
// You cannot compute a + b and look at it. Rearrange instead: if b is
// positive, the largest a that still fits is INT_MAX - b, so anything above
// that overflows. If b is negative, the smallest a that still fits is
// INT_MIN - b. If b is zero, nothing can happen.
//
// Both of those subtractions are in range for every b, which is the property
// that makes this work.
inline bool add_overflows(int a, int b) {
  (void)a;
  (void)b;
  return false;
}

// Would a - b overflow.
//
// Not the same question as add_overflows(a, -b), because negating b can
// overflow on its own when b is INT_MIN.
// TODO 2: would a - b overflow.
//
// The same rearrangement, with the signs the other way round.
//
// Do not write add_overflows(a, -b). Negating b is itself undefined when b is
// INT_MIN, so that version has the bug inside the check. The test asks
// specifically whether 1 - INT_MIN overflows, and it does.
inline bool subtract_overflows(int a, int b) {
  (void)a;
  (void)b;
  return false;
}

// Would -a overflow.
//
// Exactly one value does. Two's complement holds one more negative number than
// positive, so INT_MIN has no positive counterpart and negating it is
// undefined. This is also why std::abs(INT_MIN) is undefined, which surprises
// people who think of abs as a function that cannot fail.
// TODO 3: would -a overflow.
//
// Exactly one value does. Two's complement holds one more negative number than
// positive, so one of them has no counterpart. Which one.
inline bool negate_overflows(int a) {
  (void)a;
  return false;
}

// Would a * b overflow.
//
// Division is the way to ask without multiplying, and it needs the cases
// handled in this order. Zero first, because dividing by it is its own
// undefined behaviour. Then minus one, because INT_MIN / -1 overflows and
// would be undefined behaviour inside the very function meant to prevent it.
// After those two, every remaining division is safe.
// TODO 4: would a * b overflow.
//
// Division is how you ask without multiplying, and the order of the cases is
// the whole difficulty.
//
// Handle zero first, or you will divide by it. Handle minus one next, because
// INT_MIN / -1 overflows, and that would be undefined behaviour inside the
// function written to prevent undefined behaviour. After those two, every
// division below is safe.
//
// Then four sign combinations. For two positives, a overflows if it is above
// INT_MAX / b. For mixed signs the limit is INT_MIN, and for two negatives the
// product is positive again so the limit is INT_MAX. Work out which side of
// each comparison each one goes on; the test checks every pair in a window of
// four million, so a sign the wrong way round will not hide.
inline bool multiply_overflows(int a, int b) {
  (void)a;
  (void)b;
  return false;
}

#endif  // LESSON_SOLUTION_HPP
