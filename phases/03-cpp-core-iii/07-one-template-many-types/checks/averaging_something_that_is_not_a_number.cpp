// This must not compile.
//
// average() adds its elements up, and a Pose cannot be added. A template has
// no declared interface: it is checked against whatever the body happens to
// use, so without the static_assert this fails somewhere inside the loop and
// the message is about operator+ rather than about what the caller did wrong.
//
// With it, the first line of the error is the sentence in the assert, and it
// names this file. The lesson measures how much difference that makes.
#include "solution.hpp"

struct Pose {
  double x = 0.0;
  double y = 0.0;
};

int main() {
  Ring<Pose, 4> trail;
  trail.push(Pose{1.0, 2.0});
  return static_cast<int>(average(trail));
}
