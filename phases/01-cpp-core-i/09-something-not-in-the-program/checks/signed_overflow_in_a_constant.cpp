// This must not compile.
//
// Undefined behaviour in a constant expression makes the program ill formed,
// so the one place the compiler is required to object is where it can see the
// overflow happen at compile time. Every compiler this curriculum claims
// refuses it, and it is a hard error rather than a warning, because the
// standard says a constant expression may not contain undefined behaviour.
//
// The same expression on an unsigned type compiles cleanly, because unsigned
// wrapping is defined. The test asserts what it wraps to. Same arithmetic, two
// different rules, and that difference is the lesson.
#include <climits>

#include "solution.hpp"

int main() {
  constexpr int overflowed = INT_MAX + 1;
  return overflowed;
}
