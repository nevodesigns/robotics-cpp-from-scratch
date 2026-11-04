// This must not compile.
//
// The iterator looks up Container::value_type, because something generic has to
// be able to name the element type and cannot be told it separately.
//
// A container without that typedef cannot be iterated, cannot be passed to an
// algorithm, and cannot be used by any template that needs to declare a local
// of its element type. It is the first thing a container must say about itself,
// and lesson 03-07's Ring gained the line for exactly this reason.
#include "solution.hpp"

struct Nameless {
  double values[3] = {1.0, 2.0, 3.0};
  std::size_t size() const { return 3; }
  const double& operator[](std::size_t index) const { return values[index]; }
};

int main() {
  const Nameless samples;
  return static_cast<int>(*index_begin(samples));
}
