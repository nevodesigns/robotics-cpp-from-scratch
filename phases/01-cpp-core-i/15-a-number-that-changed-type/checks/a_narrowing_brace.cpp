// This must not compile.
//
// A double goes into an int with a plain assignment and nobody says a word:
// not gcc, not clang, not under -Wall -Wextra. The fraction is thrown away and
// the program carries on.
//
// Put the same value in with braces and the compiler refuses. Brace
// initialisation is the one place C++ checks whether a conversion loses
// information, and it checks every time, on every compiler.
//
// That makes braces the cheapest guard there is, and it is the reason to prefer
// them when a number changes type. If the loss is intended, say so with a cast
// and the reader knows it was a decision:
//
//   int whole = static_cast<int>(d);
#include "solution.hpp"

int main() {
  double d = 3.7;
  int x{d};
  return x;
}
