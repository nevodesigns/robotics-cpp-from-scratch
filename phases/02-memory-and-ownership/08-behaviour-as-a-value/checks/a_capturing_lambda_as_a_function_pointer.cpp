// This must not compile.
//
// A lambda that captures nothing is convertible to a plain function pointer,
// because there is no state to carry and a function pointer is exactly enough
// to describe it. One that captures anything is not, because the pointer has
// nowhere to put the captured values.
//
// This is worth meeting because it is the error a C API produces. A callback
// registration that wants bool(*)(double) will take your captureless lambda and
// refuse the one that closes over a limit, and the fix is a void* user pointer
// or a std::function, not a cast.
#include "solution.hpp"

int main() {
  const double limit = 1.0;
  bool (*plain)(double) = [limit](double value) { return value > limit; };
  return plain(2.0) ? 1 : 0;
}
