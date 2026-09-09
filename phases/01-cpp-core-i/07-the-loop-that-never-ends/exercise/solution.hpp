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
// TODO 1: the same count, as a signed number.
//
// One static_cast. The point is not the cast, it is having somewhere to put it
// so that the loops below read as arithmetic instead of as a conversion.
inline constexpr std::ptrdiff_t ssize(std::size_t size) {
  (void)size;
  return 0;
}

// The last valid index, or -1 when there is nothing there.
//
// This is the direct answer to size() - 1. On an empty container that
// expression is 18446744073709551615 on a 64 bit machine, and this is -1,
// which is a number a signed loop counter can compare against and stop.
// TODO 2: the last valid index, or -1 when there is nothing there.
//
// Subtract one, but not from the unsigned size. Go through ssize first, so the
// subtraction happens in signed arithmetic and can produce -1 rather than
// 18446744073709551615.
//
// The test checks both: 2 for three readings, and -1 for none.
inline constexpr std::ptrdiff_t last_index(std::size_t size) {
  (void)size;
  return 0;
}

// Is this index one you can actually use.
//
// Both ends, because the two mistakes are different. A negative index is what
// last_index hands back for an empty container, and an index equal to the size
// is the classic one past the end.
// TODO 3: is this index one you can actually use.
//
// Both ends. Below zero, which is what last_index gives for an empty
// container, and at or past the size, which is the one everybody remembers.
// Compare against ssize(size), not against size, or you have written the bug
// this lesson is about inside the function meant to catch it.
inline constexpr bool is_valid_index(std::ptrdiff_t index, std::size_t size) {
  (void)index;
  (void)size;
  return false;
}

// Does this size survive the trip through int.
//
// int n = v.size() compiles silently and is a narrowing conversion. It is
// harmless for a hundred readings and is not harmless for a point cloud, and
// the difference is not visible at the call site.
// TODO 4: does this size survive being put in an int.
//
// std::numeric_limits<int>::max() is the largest int there is. Compare against
// it, converting it to std::size_t rather than converting size down to int,
// because converting down is the thing being tested for.
inline constexpr bool fits_in_int(std::size_t size) {
  (void)size;
  return false;
}

#endif  // LESSON_SOLUTION_HPP
