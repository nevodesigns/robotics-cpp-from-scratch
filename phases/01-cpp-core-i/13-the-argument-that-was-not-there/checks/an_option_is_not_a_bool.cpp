// This must not compile.
//
// Option deliberately has no conversion to bool, and this file is that
// decision written down.
//
// If it had one, this would compile and the author would have to decide what
// it meant. Present? Present with a value? Present with a value that is not
// empty? Every one of those is a reasonable guess, two of them are wrong at
// any given call site, and whichever was chosen would be invisible in the
// code above.
//
// That is the bug this whole lesson is about, in miniature: one bit standing
// in for three answers. So the type refuses, and the call site has to say
// which question it is asking.
//
//   if (speed.present && speed.has_value)   // I need a value
//   if (speed.present)                      // I only need to know they asked
#include "solution.hpp"

int main() {
  const char* argv[] = {"drive", "--speed", "0.4", nullptr};
  const Arguments args(3, argv);

  if (args.option("--speed")) return 1;
  return 0;
}
