// This must not compile.
//
// The static_assert is the fix this lesson argues for: do not hope a type is
// cheap to move, say so, where the compiler can check it.
//
// Readings declares a destructor that does nothing. That one line suppresses
// the implicit move constructor, so every container holding these copies them
// instead of moving them, and the only visible symptom is that the program is
// slower than it was. Measured in this lesson: 198 allocations where there
// were 71.
//
// Note the message. The first line of a failed static_assert is the one that
// gets read, which lesson 03-07 measured, so it says what to do rather than
// what is wrong.
#include <type_traits>
#include <vector>

#include "solution.hpp"

struct Readings {
  std::vector<double> samples;
  ~Readings() {}
};

static_assert(std::is_nothrow_move_constructible<Readings>::value,
              "Readings must be cheap to move. Declaring a destructor suppressed "
              "its move constructor, so every container of these copies on every "
              "growth. Remove the destructor, or declare all five.");

int main() { return 0; }
