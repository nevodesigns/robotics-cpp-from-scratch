// This must not compile.
//
// Read as numbers, -1 is smaller than any size. Read the way the language
// evaluates it, the -1 becomes the largest size there is and the answer flips.
// The compiler refuses the comparison rather than picking one, which is the
// right call and is why ssize exists.
//
// The sentinel is deliberately not const. Written as const int, clang folds it
// to a known value and stops warning entirely, while gcc still refuses. That
// is measured in the lesson and it is not a detail: adding const to a variable
// removed a diagnostic on one of the two compilers.
#include <vector>

#include "solution.hpp"

int main() {
  const std::vector<double> readings = {1.0, 2.0, 3.0};
  int missing = -1;
  return (missing < readings.size()) ? 1 : 0;
}
