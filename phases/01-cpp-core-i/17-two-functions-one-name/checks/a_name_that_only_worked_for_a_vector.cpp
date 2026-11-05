// This must not compile.
//
// An unqualified call can find a function in std without anyone writing std::.
// When an argument's type belongs to a namespace, the compiler also looks in that
// namespace for the function, which is called argument dependent lookup, and the
// iterators of a std::vector belong to std.
//
// So this works, and nobody can say why it does:
//
//   std::vector<int> v{1, 2, 3};
//   auto n = distance(v.begin(), v.end());       // finds std::distance
//
// Change the container to a plain array and the arguments are int pointers, which
// belong to no namespace at all, and the same line has nothing to find:
//
//   error: 'distance' was not declared in this scope; did you mean 'std::distance'?
//
// Code that depends on this is correct for one container and broken for the next,
// and the error appears far from the change that caused it. Write std::distance.
#include <vector>

#include "solution.hpp"

int main() {
  int readings[3] = {1, 2, 3};
  const auto count = distance(readings, readings + 3);
  return static_cast<int>(count);
}
