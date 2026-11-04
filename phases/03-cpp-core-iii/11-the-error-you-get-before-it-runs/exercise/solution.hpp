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
// TODO 1: fill the array by calling compute once per index.
//
// Three lines, and all three have to be things a constant expression allows,
// or the table below stops being computable and the build fails rather than
// the test. The braces on `table{}` matter for the same reason lesson 02-09
// gave: an array left uninitialised cannot be read in a constant expression,
// and here that is a compile error rather than a silent wrong number.
template <class T, std::size_t N, class Compute>
constexpr std::array<T, N> make_table(Compute compute) {
  std::array<T, N> table{};
  (void)compute;
  return table;
}

// How many bits are set in one byte, counted the obvious way.
//
// Declared constexpr so it can be used to build the table below, and note
// what that does not mean: constexpr is permission, not instruction. This
// same function will run perfectly well at runtime on a value the compiler
// does not know, and it does, in the comparison the suite makes.
// TODO 2: count the set bits of one byte.
//
// Eight iterations, shifting and masking as lesson 01-10 did. Keep it
// constexpr: that is what lets the table be built during compilation, and
// dropping the keyword is the quickest way to discover which of the
// declarations in the suite were doing real work.
constexpr std::uint8_t bits_in_byte(std::size_t value) {
  (void)value;
  return 0;
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
// TODO 3: four lookups, one per byte of a 32 bit value.
//
// Mask out each byte in turn and add up what the table says. The masking is
// 01-10 again; the point here is that none of the 32 iterations happen.
constexpr int count_set_by_table(std::uint32_t value) {
  (void)value;
  return 0;
}

// The same answer, the way lesson 01-10 wrote it, kept so the suite can check
// the two agree on every value a byte can hold rather than on a sample.
// TODO 4: the same answer the slow way, so the suite can compare them.
//
// Thirty two iterations over the whole value. This is the version the table
// exists to replace, and the suite checks the two agree on all 256 byte
// values rather than on a sample.
constexpr int count_set_by_loop(std::uint32_t value) {
  (void)value;
  return 0;
}

#endif  // LESSON_SOLUTION_HPP
