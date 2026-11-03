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
inline bool add_overflows(int a, int b) {
  if (b > 0) return a > INT_MAX - b;
  if (b < 0) return a < INT_MIN - b;
  return false;
}

// Would a - b overflow.
//
// Not the same question as add_overflows(a, -b), because negating b can
// overflow on its own when b is INT_MIN.
inline bool subtract_overflows(int a, int b) {
  if (b < 0) return a > INT_MAX + b;
  if (b > 0) return a < INT_MIN + b;
  return false;
}

// Would -a overflow.
//
// Exactly one value does. Two's complement holds one more negative number than
// positive, so INT_MIN has no positive counterpart and negating it is
// undefined. This is also why std::abs(INT_MIN) is undefined, which surprises
// people who think of abs as a function that cannot fail.
inline bool negate_overflows(int a) { return a == INT_MIN; }

// Would a * b overflow.
//
// Division is the way to ask without multiplying, and it needs the cases
// handled in this order. Zero first, because dividing by it is its own
// undefined behaviour. Then minus one, because INT_MIN / -1 overflows and
// would be undefined behaviour inside the very function meant to prevent it.
// After those two, every remaining division is safe.
inline bool multiply_overflows(int a, int b) {
  if (a == 0 || b == 0) return false;
  if (a == -1) return negate_overflows(b);
  if (b == -1) return negate_overflows(a);

  if (a > 0) {
    if (b > 0) return a > INT_MAX / b;
    return b < INT_MIN / a;
  }
  if (b > 0) return a < INT_MIN / b;
  return a < INT_MAX / b;
}

#endif  // LESSON_SOLUTION_HPP
