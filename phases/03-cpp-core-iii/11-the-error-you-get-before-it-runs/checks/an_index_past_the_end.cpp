// This must not compile.
//
// Reading past the end of an array at runtime is undefined. If you are lucky
// the address sanitizer catches it, which is why this curriculum runs a
// sanitizer lane on every push. If you are unlucky it reads whatever is next
// in memory and the number looks plausible.
//
// In a constant expression there is no luck involved. The compiler is asked
// for the value at index 7 of a four element array, there is no such value,
// and it says so.
//
// The table in this lesson is indexed with a byte masked to 0xFF against 256
// entries, so the index cannot be out of range. Saying that in a comment is
// worth less than arranging for the compiler to check it, which is what
// using the table in a constexpr context does.
#include "solution.hpp"

#include <array>

constexpr std::array<int, 4> kSmall = {1, 2, 3, 4};

constexpr int at(std::size_t index) { return kSmall[index]; }

static_assert(at(7) == 0, "there is no element 7");

int main() { return 0; }
