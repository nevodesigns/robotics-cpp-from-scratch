// This must not compile.
//
// Slicing is the mistake that looks like nothing. Passing a Thermometer to a
// function that takes a Sensor by value copies the Sensor part of it and
// discards the rest, so the function gets an object that is the right type
// and the wrong thing. No exception, no crash, and the lesson's suite
// measures the result: 21.5 through a reference and 0 by value.
//
// Neither gcc nor clang warns, at any optimisation level, and clang says
// nothing even under -Weverything. There is no diagnostic to switch on.
//
// So the fix is not vigilance, it is making the copy unavailable. Sensor
// deletes its copy constructor and its copy assignment, which costs nothing,
// means nothing is lost, and turns this exact line into the error below.
//
// An interface is not a value. Pass it by reference or hold it behind a
// pointer, which is what you wanted in the first place, because the only
// reason it is an interface is that the thing behind it varies.
#include "solution.hpp"

static double read_a_copy(Sensor s) { return s.read(); }

int main() {
  const Thermometer t(21.5);
  return read_a_copy(t) > 0.0 ? 1 : 0;
}
