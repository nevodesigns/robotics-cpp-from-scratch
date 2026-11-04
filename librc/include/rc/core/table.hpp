// rc/core/table.hpp
//
// A lookup table built during compilation, from lesson 03-11, graduated.
//
// Everything else this curriculum measures, it measures while the program is
// running. This header is about work the compiler does instead, and the
// reason to care is not primarily speed.
//
// A constant expression has to be computable, and the rules of what is
// computable exclude undefined behaviour. Signed overflow, an index past the
// end, a division by zero and a read of something never written are all
// undefined at runtime, which is to say the compiler may assume they do not
// happen. In a constant expression it cannot assume: it has to produce the
// value, and when it cannot it says so and the build stops.
//
// That last case is the one worth remembering. Lesson 02-09 measured a struct
// member read inside a function it was passed to: no warning on gcc or clang
// at any optimisation level, the address sanitizer ran it clean and printed
// the garbage, and the memory sanitizer missed it too. The same read inside a
// constant expression is a compile error on both compilers, for free, with no
// tool and no flag.
//
// So anything you can push into a constant expression is checked for nothing.
//
// Three C++14 and later rules make make_table possible, which is why none of
// this was worth teaching in 1998: a constexpr function may contain a loop,
// std::array::operator[] is constexpr, and a lambda is implicitly constexpr
// when it only does things a constant expression allows.
//
// Measured in the lesson on a 256 entry bit count table, 100000 values, best
// of 7 interleaved rounds, against the same count written as 32 iterations:
//
//   gcc 11    -O0   6.9x faster     -O2   20.7x faster
//   clang 14  -O0   8.7x faster     -O2    7.6x faster
//
// The ratio is printed rather than asserted because it is a fact about one
// machine. The direction held in all four configurations.
//
// The first version of that measurement reported 0.0 us for both at -O2,
// because the accumulated total was never used and the optimiser deleted the
// work. A volatile store fixed it. Writing a benchmark whose result nothing
// reads is a way of timing an empty loop.
//
// On insisting. constexpr on a function is permission, not instruction: the
// same function runs happily at runtime on a value the compiler does not
// know. The way to require compile-time evaluation is to use the result where
// only a constant will do, which is what the constexpr on the table variable
// below does. Note the asymmetry, because it is the honest limit here: you
// can prove an evaluation happened at compile time, and you cannot prove one
// did not. So when it matters, demand it rather than hoping.
//
// One portability trap, which this repository has now hit five times. A
// constexpr local used inside a lambda needs no capture on gcc and clang,
// because using it for its value is not an odr-use, and MSVC rejects it as
// C3493. Audit rule L022 asks for file scope whether or not a lambda is
// involved, and E-CPP-0023 is the entry. The fifth time was while writing
// this lesson, and the rule caught it.

#ifndef RC_CORE_TABLE_HPP
#define RC_CORE_TABLE_HPP

#include <array>
#include <cstddef>

namespace rc {
namespace core {

// Builds an array by calling something once per index.
//
// The braces on table{} are not decoration: an array left uninitialised
// cannot be read in a constant expression, so without them this would not
// compile, which is lesson 02-09's defect caught at build time.
template <class T, std::size_t N, class Compute>
constexpr std::array<T, N> make_table(Compute compute) {
  std::array<T, N> table{};
  for (std::size_t i = 0; i < N; ++i) table[i] = compute(i);
  return table;
}

}  // namespace core
}  // namespace rc

#endif  // RC_CORE_TABLE_HPP
