// This must not compile.
//
// `void main()` appears in a great deal of old teaching material and in a
// fair number of forum answers, and C++ does not have it. The standard names
// exactly two signatures for main and both return int.
//
// The reason is the one this lesson is about: something is always waiting for
// that number. A program that had no way to report failure could not be used
// by a script, and almost everything that runs your robot code is a script.
//
// One mercy worth knowing, because it looks like an exception to the rule:
// main is the only function in C++ that may reach the end of its body without
// a return statement. Doing so is defined to return 0. So this is legal:
//
//   int main() { do_the_work(); }
//
// It is still better to write the return, because a reader cannot tell the
// difference between a function that succeeded and one where somebody forgot.
#include "solution.hpp"

void main() {
}
