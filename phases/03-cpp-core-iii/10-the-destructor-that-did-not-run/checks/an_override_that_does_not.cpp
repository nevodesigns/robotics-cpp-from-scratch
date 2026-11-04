// This must not compile.
//
// The signature below differs from the one it means to override by one word:
// the base declares read() const and this declares read(). That is a
// different function, so it does not override anything. It quietly becomes a
// new, non virtual member that hides the base one.
//
// Without the override keyword that compiles, and the consequence is the
// worst kind: calls through a Sensor& keep reaching the base version, calls
// on a Thermometer& reach this one, and the program behaves differently
// depending on which reference happens to be in hand. Nothing warns.
//
// The keyword is the whole defence. It costs one word per function and it
// converts that silent divergence into the error below. Put it on every
// function you intend to override, without exception, including the
// destructor.
#include "solution.hpp"

class Pyrometer : public Sensor {
 public:
  double read() override { return 900.0; }  // the base declares read() const
  const char* name() const override { return "pyrometer"; }
};

int main() {
  Pyrometer p;
  return p.read() > 0.0 ? 1 : 0;
}
