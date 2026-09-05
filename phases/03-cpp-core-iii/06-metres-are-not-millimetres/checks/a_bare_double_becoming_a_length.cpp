// This must not compile.
//
// The constructor is explicit, so a number cannot become a quantity by being
// passed to something that wanted one. Somebody has to say which unit it was
// in, at the edge, in writing.
#include "solution.hpp"

double travel(Metres distance) { return distance.value(); }

int main() { return static_cast<int>(travel(2.0)); }
