// rc/core/index.hpp
//
// Signed sizes and honest indices, from lesson 01-07, graduated.
//
// Sizes in C++ are unsigned, and unsigned arithmetic does not go below zero.
// It wraps. Every consequence of that is in these numbers, measured on a 64
// bit machine with an empty std::vector:
//
//   size()                              0
//   size() - 1       18446744073709551615
//   last_index(size())                 -1
//
// A countdown written with an unsigned index over a three element vector was
// still running after ten passes, because the condition it stops on cannot
// become false: nothing unsigned is ever less than zero, and the counter wraps
// at the bottom instead of going negative. The same countdown with a signed
// index takes three passes, and zero passes over an empty container.
//
// The comparison flips too. Read as numbers, -1 is less than 3. Converted the
// way the language actually converts it, (std::size_t)(-1) is
// 18446744073709551615 and is not less than 3.
//
// What the compiler does about it, measured under -Wall -Wextra:
//
//                                 gcc     clang   MSVC
//   int i < v.size()              warns   warns   silent
//   -1 < v.size()                 warns   warns   silent
//   the same, -1 written const    warns   silent  silent
//   unsigned countdown, i >= 0    warns   silent  silent
//   v[v.size() - 1]               silent  silent  silent
//   int n = v.size()              silent  silent  silent
//
// gcc and clang at -Wall -Wextra -Werror, MSVC at /W4 /WX. No row is caught by
// all three, and the arithmetic is caught by none of them. size() - 1 is the
// dangerous one and it is the one nothing anywhere mentions.
//
// So these functions are not a convenience over a diagnostic that would have
// saved you. There is no such diagnostic.
//
// C++20 has std::ssize. This is the same thing for the C++17 baseline, named
// the same so the day the curriculum moves the change is one line.

#ifndef RC_CORE_INDEX_HPP
#define RC_CORE_INDEX_HPP

#include <cstddef>
#include <limits>

namespace rc {
namespace core {

// The size of a container as a signed count.
inline constexpr std::ptrdiff_t ssize(std::size_t size) {
  return static_cast<std::ptrdiff_t>(size);
}

// The last valid index, or -1 when there is nothing there.
//
// The direct answer to size() - 1, which on an empty container is the largest
// number a std::size_t can hold rather than the -1 everybody reads it as.
inline constexpr std::ptrdiff_t last_index(std::size_t size) {
  return ssize(size) - 1;
}

// Is this index one you can actually use.
//
// Both ends, because the two mistakes are different. Negative is what
// last_index hands back for an empty container, and equal to the size is the
// classic one past the end.
inline constexpr bool is_valid_index(std::ptrdiff_t index, std::size_t size) {
  return index >= 0 && index < ssize(size);
}

// Does this size survive the trip through int.
//
// int n = v.size() compiles silently on every compiler this curriculum claims.
// It is harmless for a hundred readings and it is not harmless for a point
// cloud, and nothing at the call site distinguishes the two.
inline constexpr bool fits_in_int(std::size_t size) {
  return size <= static_cast<std::size_t>(std::numeric_limits<int>::max());
}

}  // namespace core
}  // namespace rc

#endif  // RC_CORE_INDEX_HPP
