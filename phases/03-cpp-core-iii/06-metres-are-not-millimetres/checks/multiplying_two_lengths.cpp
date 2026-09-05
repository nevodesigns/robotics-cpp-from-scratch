// This must not compile.
//
// A length times a length is an area, which this system has no name for, so it
// refuses rather than silently producing a length. Dividing two lengths is
// allowed, because the answer is a plain number and that is a ratio.
#include "solution.hpp"

int main() {
  const Metres width = metres(2.0);
  const Metres height = metres(3.0);
  const auto area = width * height;
  return static_cast<int>(area.value());
}
