#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cstddef>
#include <limits>

// Sizes in C++ are unsigned, and unsigned arithmetic does not go below zero.
// It wraps. So size() - 1 on an empty container is not -1, it is the largest
// number a std::size_t can hold, and every loop and index built on it is wrong
// in a way that no warning mentions.
//
// These four functions are the whole repair. They are small on purpose: the
// value is not in the code, it is in never writing size() - 1 again.

// The size of a container as a signed count.
//
// The name is the one C++20 gives this in the standard library as std::ssize.
// Writing it here means the loops in this curriculum can be correct now, and
// the day the whole thing moves to C++20 this becomes a one line change rather
// than a hunt.
inline constexpr std::ptrdiff_t ssize(std::size_t size) {
  return static_cast<std::ptrdiff_t>(size);
}

// The last valid index, or -1 when there is nothing there.
//
// This is the direct answer to size() - 1. On an empty container that
// expression is 18446744073709551615 on a 64 bit machine, and this is -1,
// which is a number a signed loop counter can compare against and stop.
inline constexpr std::ptrdiff_t last_index(std::size_t size) {
  return ssize(size) - 1;
}

// Is this index one you can actually use.
//
// Both ends, because the two mistakes are different. A negative index is what
// last_index hands back for an empty container, and an index equal to the size
// is the classic one past the end.
inline constexpr bool is_valid_index(std::ptrdiff_t index, std::size_t size) {
  return index >= 0 && index < ssize(size);
}

// Does this size survive the trip through int.
//
// int n = v.size() compiles silently and is a narrowing conversion. It is
// harmless for a hundred readings and is not harmless for a point cloud, and
// the difference is not visible at the call site.
inline constexpr bool fits_in_int(std::size_t size) {
  return size <= static_cast<std::size_t>(std::numeric_limits<int>::max());
}

#endif  // LESSON_SOLUTION_HPP
