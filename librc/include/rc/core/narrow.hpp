// rc/core/narrow.hpp
//
// Converting between number types without losing the answer silently, from
// lesson 01-15, graduated.
//
// A number changes type all the time in this curriculum. An ADC count becomes
// volts, a position becomes a grid cell, a timestamp becomes a plain int. The
// language does the conversion without being asked, and most of the time it is
// right. The times it is not, it says nothing, and the wrong number is
// plausible.
//
// Everything below is measured, on gcc 11 and clang 14 at -O0 and -O2.
//
// A cast toward int throws the fraction away TOWARD ZERO. It is not rounding
// and it is not flooring, and for a negative number the two differ:
//
//   value    static_cast   floor   round
//     2.9         2          2       3
//    -2.9        -2         -3      -3
//    -0.5         0         -1      -1
//
// Adding a half first, the usual repair, is right for positives only. Over 601
// values from -3.00 to 3.00 it disagreed with round on 251, every one of them
// negative.
//
// Truncation toward zero also gives zero two sides. On a map of one metre
// cells, twenty positions from -1.0 to 0.9 landed 19 in cell 0 and 1 in cell
// -1 by cast, and 10 and 10 by floor. The cell at the origin is twice the size
// of every other.
//
// A double that does not fit an int has no answer, and the cast gives one
// anyway. The raw cast of 1e10, -1e10, nan, inf and 2147483648.0 all gave
// -2147483648, on both compilers at both optimisation levels, so not a number
// and a real reading of the most negative int are the same value. It is
// undefined behaviour. The sanitizers disagree about it: gcc's
// -fsanitize=undefined reported nothing at all, and clang's reported it. The
// separate -fsanitize=float-cast-overflow reported it on both, so a project
// whose sanitizer lane is gcc needs to ask for it by name.
//
// Neither compiler said anything about double to int under -Wall -Wextra.
// -Wconversion does, on both, and is noisy enough that lesson 04-08 is about
// deciding whether to believe it.
//
// A 64 bit value in a 32 bit int wraps, and for the conversion down that is
// defined by the implementation, not the standard, so it is a believable
// negative number and not a crash. A microsecond clock held in an int reads
// 2147000000 at 2147 seconds and -2146967296 at 2148, which is 35.8 minutes. A
// robot that has been running for 36 minutes has a negative timestamp.
//
// A float holds every whole number exactly up to 16777216, which is 2 to the
// 24th, and not above it. 16777217 comes back as 16777216 and 100000001 as
// 100000000. Of 100 integers from 16777216 up, 50 were lost. A double holds
// every 32 bit integer: none of 65536 samples across the whole range was lost.
// An encoder count stored as a float is exact for a while and then quietly
// stops being so.
//
// Braces are the one place the language checks. int x{d} for a double d is
// refused by every compiler, which is what checks/ proves.
//
// The fix in each case is the same: make the decision visible and make the
// failure a value. These functions say whether the conversion was possible
// and what it gave, so a number that does not fit is something the caller has
// to look at.

#ifndef RC_CORE_NARROW_HPP
#define RC_CORE_NARROW_HPP

#include <cmath>
#include <limits>

namespace rc {
namespace core {

// A conversion that may not have been possible. ok is first because the value
// is meaningless without it: every int is a reading some sensor could produce,
// so no value can stand for "it did not fit".
struct IntResult {
  bool ok = false;
  int value = 0;
};

// Whether a double is a number an int can hold. NaN fails both comparisons, so
// it is refused here without a special case.
inline bool fits_int(double value) {
  return value >= static_cast<double>(std::numeric_limits<int>::min()) &&
         value <= static_cast<double>(std::numeric_limits<int>::max());
}

// The largest whole number not above value, which is what a grid cell wants.
// floor(-0.5) is -1, so negative positions land in negative cells.
inline IntResult floor_to_int(double value) {
  IntResult result;
  const double whole = std::floor(value);
  if (!fits_int(whole)) return result;
  result.ok = true;
  result.value = static_cast<int>(whole);
  return result;
}

// The nearest whole number, with halves going away from zero: 2.5 is 3 and
// -2.5 is -3. Not static_cast<int>(value + 0.5), which is wrong below zero.
inline IntResult round_to_int(double value) {
  IntResult result;
  const double whole = std::round(value);
  if (!fits_int(whole)) return result;
  result.ok = true;
  result.value = static_cast<int>(whole);
  return result;
}

// A 64 bit value that fits in an int, or the statement that it does not.
inline IntResult to_int(long long value) {
  IntResult result;
  if (value < static_cast<long long>(std::numeric_limits<int>::min())) return result;
  if (value > static_cast<long long>(std::numeric_limits<int>::max())) return result;
  result.ok = true;
  result.value = static_cast<int>(value);
  return result;
}

// Whether an int comes back the same after being held in a float.
//
// Compared as doubles, which hold every int exactly, so the question is asked
// without a conversion that could itself lose something.
inline bool survives_float(int value) {
  return static_cast<double>(static_cast<float>(value)) == static_cast<double>(value);
}

}  // namespace core
}  // namespace rc

#endif  // RC_CORE_NARROW_HPP
