// This must not compile.
//
// The loop counter is an int and the size is unsigned, so the comparison
// converts one of them, and which one is not obvious from reading it. Every
// compiler this curriculum claims refuses it, which makes this the friendly
// half of the problem: the dangerous half is size() - 1, which nothing warns
// about at all.
#include <vector>

#include "solution.hpp"

int main() {
  const std::vector<double> readings = {1.0, 2.0, 3.0};
  int seen = 0;
  for (int index = 0; index < readings.size(); ++index) ++seen;
  return seen;
}
