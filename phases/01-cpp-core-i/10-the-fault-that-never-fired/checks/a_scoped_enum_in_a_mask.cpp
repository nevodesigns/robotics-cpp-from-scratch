// This must not compile.
//
// A scoped enum does not implicitly convert to its underlying type, so it
// cannot be dropped into a bitwise expression. That is a hard type error on
// every compiler, because the standard says there is no conversion, rather
// than a warning somebody can switch off.
//
// It is worth meeting deliberately, because it is the one place the type
// system can help with flags at all. Two different registers on the same
// device, each with its own set of fault bits, are both plain integers to the
// compiler, and nothing stops a mask for one being tested against the other.
// A scoped enum per register stops exactly that, and the price is that the
// conversion has to be written where it is meant.
#include <cstdint>

#include "solution.hpp"

enum class Fault : std::uint32_t {
  OverTemp = 1u << 1,
  Stalled = 1u << 2,
};

int main() {
  const Bits status = 0x06u;
  return has_any(status, Fault::OverTemp) ? 1 : 0;
}
