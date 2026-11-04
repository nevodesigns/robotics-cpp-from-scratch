// This must not compile.
//
// Arguments::Number does not convert to double, for the same reason Option
// does not convert to bool.
//
// A command line is text a person typed, so every number on one has three
// possible fates before it is a number: never given, given with nothing
// after it, given as something that is not a number. A function that handed
// back a plain double would have to pick a value to mean all three, and the
// value available is 0.
//
// Zero is a speed. It is a gain, an offset and a timeout. Returning it for
// "you did not tell me" is how a tool ends up doing nothing at all and
// reporting success, which is the hardest kind of failure to diagnose because
// there is nothing to see.
//
// So read the flags first and the value second:
//
//   const Arguments::Number speed = args.number("--speed");
//   if (!speed.present) return usage("--speed is required");
//   if (!speed.ok) return usage("--speed needs a number");
//   drive(speed.value);
#include "solution.hpp"

int main() {
  const char* argv[] = {"drive", "--speed", "0.4", nullptr};
  const Arguments args(3, argv);

  const double speed = args.number("--speed");
  return speed > 0.0 ? 1 : 0;
}
