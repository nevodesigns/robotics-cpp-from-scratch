// This must not compile.
//
// Lesson 00-04 measured what signed overflow does at runtime: it wraps, or it
// does something else, because it is undefined and the compiler is entitled
// to assume it cannot happen. Lesson 01-13 measured atoi turning a twenty
// digit number into -1 for the same reason.
//
// In a constant expression the compiler cannot assume anything. It has to
// produce the value, there is no value to produce, and so it stops. The same
// arithmetic, in a context that demands an answer, is a build failure instead
// of a number nobody checked.
//
// That is the whole argument for computing what you can at compile time, and
// it is not about speed. Undefined behaviour is not a constant expression, so
// anything you can push into a constant expression is checked for free.
#include "solution.hpp"

constexpr int scaled(int value) { return value * 1000000; }

static_assert(scaled(3000) > 0, "3000 million does not fit in an int");

int main() { return 0; }
