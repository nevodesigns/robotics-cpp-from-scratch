// The definitions belong here.
//
// One of the two functions declared in solution.hpp is defined below. The other
// is missing, which is why this lesson does not link yet. That linker error is
// the lesson: read it, run rcpp explain on it, then write the missing body.

#include "solution.hpp"

double celsius_from_raw(int raw) {
  return raw * 0.0625;
}

// TODO: define is_overheating here.
// Write it in terms of celsius_from_raw rather than repeating the arithmetic,
// so there is only one place to fix if the sensor scale ever changes.
