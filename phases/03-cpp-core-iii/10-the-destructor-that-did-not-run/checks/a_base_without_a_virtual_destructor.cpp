// This must not compile.
//
// This is the one trap in the lesson that cannot be caught at runtime.
//
// Deleting a derived object through a pointer to a base whose destructor is
// not virtual is undefined behaviour. The derived destructor does not run, so
// whatever it was going to release is not released, and a test that
// demonstrated it would be a test that performs undefined behaviour: the
// address sanitizer stops the program with new-delete-type-mismatch, which
// is the right thing for it to do and no use as a measurement.
//
// There is also frequently no warning. gcc warns as -Wdelete-non-virtual-dtor
// when the base has some other virtual function, because then it knows the
// class is polymorphic. A base with no virtual functions at all, which is
// what a handle or a resource wrapper usually looks like, is silent on both
// gcc and clang.
//
// So do not test for it. Require it. One static_assert per interface, next to
// the declaration, turns a trap that costs an afternoon into a build failure
// that costs nothing. The suite asserts the same thing about Sensor.
#include "solution.hpp"

#include <type_traits>

class Handle {
 public:
  ~Handle() {}  // not virtual
  int id = 0;
};

static_assert(std::has_virtual_destructor<Handle>::value,
              "a base that anything is deleted through needs a virtual destructor");

int main() { return 0; }
