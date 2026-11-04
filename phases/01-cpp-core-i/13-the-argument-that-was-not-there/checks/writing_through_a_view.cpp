// This must not compile.
//
// Arguments hands back std::string_view, which is a pointer and a length
// pointing at memory somebody else owns. In this case that somebody is the
// operating system, which put the command line there before main started.
//
// A view is read only, so this assignment is refused, and that refusal is
// worth meeting on purpose because the other half of the same fact has no
// diagnostic at all: a view does not keep the thing it points at alive.
//
// These are fine, because argv outlives main:
//
//   const Arguments args(argc, argv);
//   const std::string_view name = args.option("--mode").value;
//
// This is not, and nothing will tell you:
//
//   std::string_view bad = std::string("fast");   // dangles immediately
//
// Lesson 01-05 introduced the view and lesson 02-07 is the full account of
// what happens when the thing underneath goes away.
#include "solution.hpp"

int main() {
  const char* argv[] = {"drive", "--mode", "fast", nullptr};
  const Arguments args(3, argv);

  args[0][0] = 'x';
  return 0;
}
