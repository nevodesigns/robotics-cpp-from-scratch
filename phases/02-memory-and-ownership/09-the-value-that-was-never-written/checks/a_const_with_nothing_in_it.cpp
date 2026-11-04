// This must not compile.
//
// The language refuses exactly one case of the thing this whole lesson is
// about, and it is worth knowing which one, because the boundary tells you
// what the compiler is actually able to reason about.
//
// A const object can never be assigned to after it is created. So an
// uninitialised one is not a value that might be written later, the way an
// ordinary local is: it is a value that can never be written at all, and
// every read of it for the rest of the program is a read of leftover memory.
// That is unambiguous, local, and decidable, so it is an error.
//
// Drop the const and the same declaration is accepted everywhere, because now
// the compiler would have to prove that no path writes it before the first
// read, and across a function boundary it cannot. That is why the diagnostics
// measured in the lesson stop where they do.
//
// The fix is not to remove the const. It is `const Pose p = {};` or, better,
// members with default initialisers, which lesson 01-01 asked for and this
// lesson measures the second reason for.
#include "solution.hpp"

struct Pose {
  double x;
  double y;
  double heading;
};

int main() {
  const Pose p;
  return p.heading > 0.0 ? 1 : 0;
}
