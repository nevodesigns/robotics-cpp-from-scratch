// This must not compile.
//
// A ring with no room is not a smaller ring. Every operation on it would be an
// error: there is nowhere to push to, nothing to read, and the modular
// arithmetic inside it would divide by zero.
//
// So the template refuses it, with a static_assert that names the problem in
// one sentence. The alternative is a type that compiles and divides by zero at
// the first push, which lesson 01-09 covered.
#include "solution.hpp"

int main() {
  Ring<double, 0> nowhere;
  return static_cast<int>(nowhere.capacity());
}
