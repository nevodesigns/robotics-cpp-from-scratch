// This must not compile.
//
// Two subsystems each have a function called limit, which is the whole reason
// they live in namespaces: drive::limit keeps a speed in range and arm::limit
// keeps an angle in range, and neither has to be called drive_limit.
//
// A using-directive makes every name in a namespace visible without its prefix.
// Make two of them visible at once and the unqualified call has two equally good
// candidates, and the compiler refuses to pick:
//
//   error: call of overloaded 'limit(double)' is ambiguous       (gcc)
//   error: call to 'limit' is ambiguous                           (clang)
//
// This is the good outcome. It is loud, it is at the line that is wrong, and it
// is fixed by writing the prefix. The bad outcome is the one the suite measures,
// where the two candidates are not equally good and the compiler picks one
// without a word.
#include "solution.hpp"

using namespace drive;
using namespace arm;

int main() {
  return static_cast<int>(limit(1.0));
}
