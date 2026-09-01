// rc/core/convert.hpp
//
// The sensor conversions from phase 00, graduated: the battery reading from
// lesson 00-01, the temperature from 00-02, and the rest from 00-04, which is
// the first place the arithmetic lies.
//
// Whole number division throws the remainder away, so a percentage computed by
// dividing first is zero for every input below the maximum. Multiply before
// dividing, and know which of the two you are doing.

#ifndef RC_CORE_CONVERT_HPP
#define RC_CORE_CONVERT_HPP

#include <cstddef>

namespace rc {
namespace core {

inline int battery_percent(int millivolts) {
  const int empty_mv = 3000;
  const int full_mv = 4200;

  // Handle the ends first. Doing this before any arithmetic means the rest of
  // the function never has to worry about producing a number outside 0 to 100.
  if (millivolts <= empty_mv) return 0;
  if (millivolts >= full_mv) return 100;

  // How far above empty are we, and how wide is the whole range?
  const int above_empty = millivolts - empty_mv;
  const int full_range = full_mv - empty_mv;

  // Multiply before dividing. Whole number division throws away the fraction,
  // so (above_empty / full_range) would be 0 for every reading below full.
  // Lesson 00-03 is entirely about this trap.
  return (above_empty * 100) / full_range;
}

inline double celsius_from_raw(int raw) {
  return raw * 0.0625;
}

// Built on top of celsius_from_raw rather than repeating 0.0625. If the sensor
// is ever swapped for one with a different scale, exactly one line changes.
inline bool is_overheating(int raw) {
  return celsius_from_raw(raw) > 80.0;
}

inline double adc_to_volts(int raw) {
  // One fractional operand makes the whole expression fractional, so the
  // division keeps its remainder. Writing 4095.0 rather than 4095 is the entire
  // fix, and it is easy to miss in review, which is why it is worth a comment.
  return (raw * 3.3) / 4095.0;
}

inline int percent_of(int part, int whole) {
  // Guard the impossible input before doing arithmetic on it. Dividing by zero
  // with whole numbers is undefined behaviour, which means the program is
  // allowed to do anything at all, including appearing to work.
  if (whole == 0) return 0;

  // Multiply before dividing so the ratio survives, then add half a divisor to
  // round to nearest instead of always rounding towards zero.
  return (part * 100 + whole / 2) / whole;
}

inline double average(const int* samples, int count) {
  if (count <= 0) return 0.0;

  // The running total is a fractional number, so the final division is too.
  // Summing into a double also avoids overflowing an int on a long run of
  // large samples.
  double sum = 0.0;
  for (int i = 0; i < count; ++i) {
    sum += samples[i];
  }
  return sum / count;
}

}  // namespace core
}  // namespace rc

#endif  // RC_CORE_CONVERT_HPP
