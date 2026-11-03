// rc/core/checked.hpp
//
// Asking whether an integer operation would overflow, before it does, from
// lesson 01-09, graduated.
//
// Lesson 01-07 was about unsigned arithmetic, which wraps. Wrapping is defined
// behaviour: the answer is wrong, and it is the same wrong answer on every
// machine and every compiler, so it can be found.
//
// Signed overflow is a different thing. It is undefined, which does not mean
// the result is unpredictable. It means the standard places no requirement on
// the program at all, and the compiler is entitled to assume it never happens.
//
// What that costs, measured. A function returning x + 1 > x, called with
// INT_MAX, returned true on gcc and clang at -O0, -O1 and -O2: an answer that
// is wrong for the only int it was asked about. And a loop doubling an int
// from 1 while i > 0, with a counter bounded at 200 so it could be reported:
//
//   g++   -O0   31 doublings
//   g++   -O1   31
//   g++   -O2   201   (the bound, reached)
//   clang -O2   201   (the bound, reached)
//
// At -O2 both compilers deleted the exit test, because an int doubling from 1
// is positive unless it overflows, and overflow is assumed not to occur.
// Without the bound the loop does not end. The source did not change.
//
// So there is no checking after the fact. By the time a + b has overflowed
// there is no answer to inspect, and a test for a negative result is a test
// the compiler is permitted to delete. The question is asked first, and these
// four functions are how.
//
// Each reaches its answer without performing the operation it asks about, and
// the lesson's suite proves it two ways: 14065212 predictions across four
// exhaustive sweeps with none wrong, and the whole boundary set run under the
// undefined behaviour sanitizer with nothing reported.
//
// The undefined behaviour sanitizer finds these at runtime, on the paths a
// test actually exercises, and names the operation, the file and the line:
//
//   runtime error: signed integer overflow: 1073741824 * 2 cannot be
//   represented in type 'int'
//
// which is why this curriculum runs a sanitizer lane on every push.

#ifndef RC_CORE_CHECKED_HPP
#define RC_CORE_CHECKED_HPP

#include <climits>

namespace rc {
namespace core {

// Would a + b overflow.
//
// Rearranged so the arithmetic stays in range: rather than computing a + b and
// looking at it, compare a against the largest value that can still take b.
inline bool add_overflows(int a, int b) {
  if (b > 0) return a > INT_MAX - b;
  if (b < 0) return a < INT_MIN - b;
  return false;
}

// Would a - b overflow.
//
// Not the same question as add_overflows(a, -b), because negating b is itself
// undefined when b is INT_MIN.
inline bool subtract_overflows(int a, int b) {
  if (b < 0) return a > INT_MAX + b;
  if (b > 0) return a < INT_MIN + b;
  return false;
}

// Would -a overflow.
//
// Exactly one value does. Two's complement holds one more negative number than
// positive, so INT_MIN has no positive counterpart. This is also why
// std::abs(INT_MIN) is undefined, which surprises people who think of abs as a
// function that cannot fail.
inline bool negate_overflows(int a) { return a == INT_MIN; }

// Would a * b overflow.
//
// Division is how to ask without multiplying, and the order of the cases is
// the whole difficulty. Zero first, or the division below divides by it. Then
// minus one, because INT_MIN / -1 overflows and would be undefined behaviour
// inside the function written to prevent it. After those two every remaining
// division is in range.
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

}  // namespace core
}  // namespace rc

#endif  // RC_CORE_CHECKED_HPP
