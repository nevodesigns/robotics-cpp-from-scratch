// This must not compile.
//
// This is the one to remember, because it is the bug lesson 02-09 could not
// catch with any tool.
//
// There, a struct member read inside a function it was passed to produced no
// warning on gcc or clang at any optimisation level, and the address
// sanitizer ran the program clean and printed the garbage. The memory
// sanitizer is clang only, cannot be combined with the address sanitizer,
// needs an instrumented standard library, and on this machine missed that
// exact case.
//
// In a constant expression it is caught by both compilers, for free, with no
// tool and no flag:
//
//   gcc:   uninitialized variable 'x' in 'constexpr' function
//   clang: uninitialized variable in a constexpr function is a C++20 extension
//
// Clang's wording is the more informative one. C++20 relaxed the rule so the
// declaration itself is allowed; reading one during constant evaluation is
// still not a constant expression, so the error moves rather than
// disappearing. This curriculum is C++17, where the declaration is refused
// outright.
#include "solution.hpp"

constexpr int never_written() {
  int x;
  return x;
}

static_assert(never_written() == 0, "nothing was ever put in x");

int main() { return 0; }
