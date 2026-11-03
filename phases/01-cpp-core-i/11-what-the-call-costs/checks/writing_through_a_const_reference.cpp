// This must not compile.
//
// A const reference parameter is a promise to the caller that their data will
// come back unchanged, and the compiler holds you to it as a type error rather
// than a diagnostic. That promise is the reason a const reference is safe to
// hand out: borrowing without it means reading the function body to find out
// whether your vector survived the call.
#include <vector>

#include "solution.hpp"

void scale_in_place(const std::vector<double>& samples) { samples[0] = 2.0; }

int main() {
  std::vector<double> readings(4, 1.0);
  scale_in_place(readings);
  return 0;
}
