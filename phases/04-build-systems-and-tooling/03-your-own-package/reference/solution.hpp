#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

// The library itself, so the lesson's own tests can check it directly. The C++
// is finished and is not what this lesson is about.
//
// What you write is:
//
//   package/CMakeLists.txt    build the library and install it as a package
//   consumer/CMakeLists.txt   find that package and link to it
//
// The test builds and installs your package into a temporary directory, then
// deletes the sources, then builds the consumer against what was installed. A
// package that only works while its source tree is still there passes nothing.
#include "package/include/steplib/step.hpp"

#endif  // LESSON_SOLUTION_HPP
