// This must not compile.
//
// Shifting by the width of the type, or further, is undefined. Not
// implementation defined, not zero: undefined. An int is 32 bits on every
// platform this curriculum claims, so a shift of 32 is already too far.
//
// This is the one people meet while writing a bit mask for a sensor register,
// where the obvious loop runs the shift count up to the width inclusive.
#include "solution.hpp"

int main() {
  constexpr int shifted = 1 << 32;
  return shifted;
}
