// This must not compile.
//
// A by value capture is const inside the lambda. The lambda owns that copy, so
// it looks as though it should be free to change it, and it is not: the call
// operator a lambda generates is const unless you write mutable.
//
// That default is the right one. A lambda you can call twice and get different
// answers from is a lambda with state, and state you did not ask for is the
// thing this curriculum spends phase 05 testing for. Writing mutable is how you
// say you meant it.
#include "solution.hpp"

int main() {
  int readings_seen = 0;
  const auto count_one = [readings_seen]() { return ++readings_seen; };
  return count_one();
}
