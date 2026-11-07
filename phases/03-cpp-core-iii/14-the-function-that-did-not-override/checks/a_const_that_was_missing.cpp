// This must not compile.
//
// A controller overrides output() and leaves off the const that the base version has. Without
// `override` that is a different function: the base one is not replaced, and a call through a
// reference to the base class runs the base version, which does nothing. It builds, it runs, and
// the controller is silently dead.
//
// With `override` the compiler is asked to confirm that this replaces something, and it says it does not.
#include "solution.hpp"

class Forgetful : public Controller {
 public:
  double output(double error) override { return 2.0 * error; }   // no const: not the same function
};

int main() {
  Forgetful f;
  return static_cast<int>(f.output(1.0)) - 2;
}
