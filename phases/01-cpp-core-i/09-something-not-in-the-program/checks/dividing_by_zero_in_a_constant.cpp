// This must not compile.
//
// Integer division by zero is undefined behaviour, and unlike the floating
// point case there is no infinity for it to produce. On x86 it raises a
// hardware fault that no catch block can see, which lesson 00-04 met from the
// other side.
//
// Here the divisor is a constant, so the compiler can see it and must object.
// When the divisor arrives from a sensor at runtime, nothing objects, and the
// guard has to be written.
#include "solution.hpp"

int main() {
  constexpr int divided = 1 / 0;
  return divided;
}
