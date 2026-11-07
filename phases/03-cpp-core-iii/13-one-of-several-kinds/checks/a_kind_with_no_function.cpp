// This must not compile.
//
// A fourth kind of command is added, and the function that applies commands is not told. With a
// type field and a switch that would build and do nothing for the new kind, at three in the morning,
// on the robot. With std::visit the call has no function for the new kind, and the compiler stops.
#include "solution.hpp"

struct Park {};
using BiggerCommand = std::variant<Stop, Drive, SetLimit, Park>;

// The same Applier as the lesson's, which knows three kinds.
int main() {
  RobotState state;
  BiggerCommand command = Park{};
  std::visit(Applier{state}, command);
  return 0;
}
