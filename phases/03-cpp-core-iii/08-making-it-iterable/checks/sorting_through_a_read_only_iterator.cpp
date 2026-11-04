// This must not compile.
//
// std::sort rearranges the elements, so it has to write through the iterator it
// is given. This one hands out const references, because it exists to read a
// history that has already happened.
//
// The refusal is the type system saying what the design decided: a window of
// readings you have collected is not a thing to sort in place. Copy it into a
// std::vector and sort that, which costs an allocation and says plainly that a
// sorted copy is a different object from the history.
//
// Worth meeting deliberately, because the error arrives from inside
// <algorithm> and lesson 03-07 measured how far from your own code that can be.
#include <algorithm>
#include <vector>

#include "solution.hpp"

struct Samples {
  using value_type = double;
  std::vector<double> values;
  std::size_t size() const { return values.size(); }
  const double& operator[](std::size_t index) const { return values[index]; }
};

int main() {
  const Samples samples{{3.0, 1.0, 2.0}};
  std::sort(index_begin(samples), index_end(samples));
  return 0;
}
