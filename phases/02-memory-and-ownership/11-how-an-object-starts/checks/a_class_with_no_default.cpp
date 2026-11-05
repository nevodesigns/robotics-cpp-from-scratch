// This must not compile.
//
// A Meters has to be given a value. There is no such thing as a distance that
// has not been measured, and a default of zero would be a lie that looks like a
// reading: a robot at zero metres from the wall.
//
// So the class declares a constructor that takes a number and declares no other,
// and the compiler withdraws the one it would have written. This line then has
// nothing to call:
//
//   error: no matching function for call to 'Meters::Meters()'
//
// The refusal is the design. A class that WANTS a no-argument form says so, with
// = default, and the exercise's Ramp is the example of when that is right.
#include "solution.hpp"

int main() {
  Meters distance;
  return static_cast<int>(distance.value());
}
