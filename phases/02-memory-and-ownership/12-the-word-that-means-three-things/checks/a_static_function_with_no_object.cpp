// This must not compile.
//
// A static member function belongs to the class and not to any object of it. It
// can be called with no object at all, Wheel::count(), which is its whole use.
// That means it has no this, and no member that is part of an object has any
// meaning inside it.
//
//   error: invalid use of member 'Wheel::radius_' in static member function   (gcc)
//   error: invalid use of member 'radius_' in static member function          (clang)
//
// Whose radius? There is no wheel. The refusal is the class telling the truth
// about what the function is.
//
// The reverse is fine. An ordinary member function may use a static member,
// because the class-wide counter exists whichever object is asking.
#include "solution.hpp"

struct Broken {
  double radius_ = 0.0;
  static double doubled() { return radius_ * 2.0; }
};

int main() {
  return static_cast<int>(Broken::doubled());
}
