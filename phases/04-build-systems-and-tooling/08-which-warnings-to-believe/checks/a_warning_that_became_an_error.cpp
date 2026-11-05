// This must not compile.
//
// The whole lesson in one file: `limit` is never used, which is a warning and
// not an error, and this directory is compiled with -Werror, which makes it
// one.
//
// Without that flag the same file builds and exits 0. Measured:
//
//   g++ -Wall -Wextra          warning: unused parameter 'limit'   exit 0
//   g++ -Wall -Wextra -Werror  error: unused parameter 'limit'     exit 1
//
// Same compiler, same source, same diagnostic, two outcomes. That is worth
// meeting on purpose, because it is the only thing standing between a
// warning and being ignored.
//
// Why this curriculum does not use -Werror on the lesson targets, only here:
// a learner's first encounter with a warning should be a message they can read
// and decide about, not a build that refuses to run. A course that fails your
// build on a warning in lesson four teaches you to switch warnings off.
//
// A real project should turn it on, and the discipline that makes that
// bearable is in the lesson: count sites rather than lines, and keep a list
// of the ones you investigated and dismissed, so a new warning is an event.
#include "solution.hpp"

int drive(double speed, double limit) {
  return speed > 0.0 ? 1 : 0;
}

int main() { return drive(1.0, 2.0); }
