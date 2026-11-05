// This must not compile.
//
// A vector of bool does not store bools. To pack eight flags into a byte it
// stores bits, and indexing it hands back a small object that stands for one
// of them, not a reference to a bool, because there is no bool to refer to.
//
// A loop variable that is a plain reference wants exactly that, a reference to
// something that exists, and the compiler refuses. It says so with a message
// about binding to a temporary, which is true and does not mention vector<bool>
// until you read the type in the middle of it.
//
// The loop that does work is the one that copies the proxy:
//
//   for (auto flag : flags) count += flag;
//
// and writing to the flags is an index loop, which is what the exercise asks.
#include <vector>

#include "solution.hpp"

int main() {
  std::vector<bool> flags(3, true);
  int count = 0;
  for (auto& flag : flags) count += flag;
  return count;
}
