// This must not compile.
//
// A seqlock copies its value as raw bytes, a few words at a time, while a writer may
// be changing them, and then throws the copy away if the writer interfered. That is
// only meaningful for a type whose bytes ARE its value: a plain struct of numbers.
//
// A std::string is a pointer and a length to somewhere else. A byte copy taken in the
// middle of a write could hold a pointer to memory the writer has just freed, and the
// seqlock would then be discarding a copy that had already been dereferenced.
//
// So the class refuses at compile time, with a static_assert on
// std::is_trivially_copyable, and not at run time with a crash.
#include <string>

#include "solution.hpp"

int main() {
  Seqlock<std::string> cell;
  cell.write("hello");
  return static_cast<int>(cell.read().size());
}
