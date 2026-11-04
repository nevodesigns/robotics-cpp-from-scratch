// This must not compile.
//
// The same rule from the other direction. Declaring a move constructor
// suppresses the implicit copy constructor, so this class became move only
// without anybody deciding that it should be.
//
// Unlike the destructor case, this one is refused rather than silent, because
// the thing it took away is something the caller tries to use. The destructor
// case takes away something the compiler was going to do on your behalf and
// nothing complains.
//
// That asymmetry is worth meeting. Both are the same rule, and only one of
// them tells you.
#include <vector>

#include "solution.hpp"

struct Telemetry {
  std::vector<double> samples;
  Telemetry() = default;
  Telemetry(Telemetry&&) = default;
};

int main() {
  Telemetry original;
  Telemetry copied = original;
  return static_cast<int>(copied.samples.size());
}
