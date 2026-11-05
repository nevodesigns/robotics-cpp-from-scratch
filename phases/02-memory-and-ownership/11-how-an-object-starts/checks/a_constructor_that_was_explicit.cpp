// This must not compile.
//
// A number goes where a Meters was wanted and the compiler refuses to guess.
// Meters has one constructor that takes a number, and it is marked explicit,
// which is a promise to the reader: a bare 5.0 is not a distance until somebody
// writes Meters(5.0) and says so.
//
// Without the word, the same line compiles and 5.0 becomes a distance of five
// metres, whether or not it was ever a distance. A speed, an angle or a count
// passed to the wrong parameter is converted without a murmur, and every
// function that takes a Meters accepts every number in the program.
//
// The standard library does the same on purpose. This line does not compile
// either:
//
//   std::vector<int> v = 5;
//
// because vector's constructor taking a size is explicit, and "a vector equal
// to 5" is not a thing anybody means.
#include "solution.hpp"

int main() {
  Meters distance = 5.0;
  return static_cast<int>(distance.value());
}
