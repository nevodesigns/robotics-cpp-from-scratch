// This must not compile.
//
// The temptation is obvious and the refusal is the lesson: the way out of a
// program is a number, and there is nowhere in it to put a sentence.
//
// That is not a limitation somebody forgot to lift. The number is read by
// shells, build systems and process supervisors that have no idea what your
// program does and no way to display your text. All they can do with it is
// decide whether to carry on.
//
// So the two ways out have two jobs, and they are not interchangeable. The
// sentence goes to the second stream, where a person reads it:
//
//   std::cerr << "cannot open " << path << "\n";
//   return kFailed;
//
// Both lines. The message for the human, the number for the machine.
#include "solution.hpp"

int main() {
  return "cannot open the route file";
}
