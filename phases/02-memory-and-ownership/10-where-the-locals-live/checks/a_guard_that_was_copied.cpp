// This must not compile.
//
// A depth guard counts one level on the way in and gives it back on the way
// out. Copying one would count a level once and give it back twice, and the
// counter would drift downwards until the limit stopped meaning anything.
//
// So the copy is deleted, and this file is that decision written down. It is
// the same reasoning as lesson 05-06's Restore and lesson 03-10's interface:
// a thing that must happen exactly once should not be copyable, and the
// compiler can enforce that for free.
//
// Note what is not deleted. The guard is meant to be created as a local in
// each recursive call and destroyed when that call returns, which needs no
// copying and no moving. Passing the DepthLimit by reference is how the state
// travels; the guard itself never goes anywhere.
#include "solution.hpp"

int main() {
  DepthLimit limit(4);
  const DepthGuard outer(limit);
  const DepthGuard copied = outer;
  return copied.ok() ? 0 : 1;
}
