// This must not compile.
//
// Braces check the count in one direction only, and the asymmetry is the
// point.
//
// Too many initialisers is an error on every compiler: the type has three
// members and you wrote four, so one of them has nowhere to go and the
// mistake is certain.
//
// Too few is silently accepted and the members you left out are value
// initialised, which is to say set to zero. `Pose p = {1.5};` is a complete,
// defined Pose at x = 1.5, not one member set and two left as leftovers.
//
// So the compiler protects you from the harmless mistake and says nothing
// about the one that looks identical. Knowing which way round that is saves
// you from assuming a short brace is as dangerous as no brace, when it is in
// fact the fix.
#include "solution.hpp"

struct Pose {
  double x;
  double y;
  double heading;
};

int main() {
  Pose p = {1.5, 2.0, 3.0, 4.0};
  return p.heading > 0.0 ? 1 : 0;
}
