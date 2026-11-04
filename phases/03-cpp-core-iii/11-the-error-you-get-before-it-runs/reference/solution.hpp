#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <array>
#include <cstddef>
#include <cstdint>

// Everything measured in this curriculum so far has been measured while the
// program was running. This lesson is about work the compiler does instead,
// and about the errors you therefore get before the program exists.
//
// The headline is not speed. It is that a constant expression has to be
// computable, and the rules of what is computable exclude undefined
// behaviour. Signed overflow, an index past the end, a division by zero and a
// read of something never written are all undefined at runtime, which is to
// say the compiler may assume they do not happen. In a constant expression it
// cannot assume: it has to produce the value, and when it cannot, it says so
// and the build stops.
//
// Lesson 02-09 measured a read of an unwritten int giving 1405118043 at -O0
// and 0 at -O2, with no warning in the case that reaches a robot and nothing
// in the sanitizer lane to catch it. The same read inside a constant
// expression is a compile error on both compilers. checks/ holds it.

// Builds an array by calling something once per index.
//
// The loop is ordinary. What makes it useful is that the whole thing can run
// during compilation, so the array arrives in the program already filled in
// and nothing has to run at startup to fill it.
//
// Three things make that possible, and all three are C++14 or later, which is
// why this was not worth teaching in 1998: a constexpr function may contain a
// loop, std::array::operator[] is constexpr, and a lambda is implicitly
// constexpr when it only does things a constant expression allows.
template <class T, std::size_t N, class Compute>
constexpr std::array<T, N> make_table(Compute compute) {
  std::array<T, N> table{};
  for (std::size_t i = 0; i < N; ++i) table[i] = compute(i);
  return table;
}

// How many bits are set in one byte, counted the obvious way.
//
// Declared constexpr so it can be used to build the table below, and note
// what that does not mean: constexpr is permission, not instruction. This
// same function will run perfectly well at runtime on a value the compiler
// does not know, and it does, in the comparison the suite makes.
constexpr std::uint8_t bits_in_byte(std::size_t value) {
  std::uint8_t count = 0;
  for (std::size_t bit = 0; bit < 8; ++bit)
    if (((value >> bit) & 1u) != 0u) ++count;
  return count;
}

// The table, built during compilation.
//
// constexpr on the variable is the part that forces the issue. Written as
// `const` or with nothing at all, the compiler is permitted to build this at
// startup instead, and you would have no way to tell. Written like this, a
// build that could not do it at compile time is a build that fails.
constexpr std::array<std::uint8_t, 256> kBitsPerByte =
    make_table<std::uint8_t, 256>(bits_in_byte);

// Counting the bits of a 32 bit value, four table lookups instead of 32
// iterations.
constexpr int count_set_by_table(std::uint32_t value) {
  return kBitsPerByte[value & 0xFFu] + kBitsPerByte[(value >> 8) & 0xFFu] +
         kBitsPerByte[(value >> 16) & 0xFFu] + kBitsPerByte[(value >> 24) & 0xFFu];
}

// The same answer, the way lesson 01-10 wrote it, kept so the suite can check
// the two agree on every value a byte can hold rather than on a sample.
constexpr int count_set_by_loop(std::uint32_t value) {
  int count = 0;
  for (int bit = 0; bit < 32; ++bit)
    if (((value >> bit) & 1u) != 0u) ++count;
  return count;
}

#endif  // LESSON_SOLUTION_HPP
