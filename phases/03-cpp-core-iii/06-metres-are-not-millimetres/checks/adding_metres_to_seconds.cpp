// This must not compile.
//
// A distance and a duration are both a double underneath, and adding them is
// arithmetic that works perfectly and means nothing. Refusing it is the whole
// purpose of the type.
#include "solution.hpp"

int main() {
  const Metres distance = metres(2.0);
  const Seconds elapsed = seconds(3.0);
  const auto nonsense = distance + elapsed;
  return static_cast<int>(nonsense.value());
}
