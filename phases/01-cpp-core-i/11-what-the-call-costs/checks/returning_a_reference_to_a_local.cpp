// This must not compile.
//
// The local vector is destroyed when the function returns, so the reference
// handed back refers to a buffer that has been freed. Reading through it is
// undefined behaviour, and lesson 01-09 covered what that means.
//
// This one the compiler can see, because the lifetime of a local is written
// down in the function. It is refused rather than warned about once warnings
// are errors, which is the setting a checks/ snippet is compiled with.
//
// The reason people write it is the belief that returning a container by value
// copies it. The suite measures that belief and it is false: the buffer
// travels out. There was never anything to optimise here.
#include <vector>

#include "solution.hpp"

const std::vector<double>& newest_reading() {
  std::vector<double> local(4, 1.0);
  return local;
}

int main() { return static_cast<int>(newest_reading().size()); }
