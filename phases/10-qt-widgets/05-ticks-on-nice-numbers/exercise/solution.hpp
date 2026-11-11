#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cmath>
#include <iomanip>
#include <sstream>
#include <string>

// An axis needs numbers written on it, and a person wants them at places that are easy to read: 0, 0.2, 0.4 and not 0.137, 0.274. The step
// between them is 1, 2 or 5 times a power of ten, and the first tick is the first multiple of the step that is inside the range.
//
// Every tick is computed as a whole number times the step, and not by adding the step to the last tick, because adding 0.1 ten times is not 1.

// The step between ticks: 1, 2, 5 or 10 times a power of ten, the smallest of those that is at least span / target. A span or a target that
// is not positive gives 1.
inline double nice_step(double span, int target) {
  // TODO: if span is not positive or target < 1, return 1.0. raw = span / target. magnitude = pow(10, floor(log10(raw))). fraction = raw / magnitude.
  // The step is 1, 2, 5 or 10 times the magnitude: the smallest of them that is at least fraction. Allow 1e-9 of slack on each comparison, so that
  // 0.2 / 0.1 = 2.0000000000000004 is not pushed up to 5.
  (void)span;
  (void)target;
  return 1.0;
}

struct Ticks {
  long long first_index = 0;   // the first tick is first_index * step
  double step = 1.0;
  int count = 0;
  int decimals = 0;            // places after the point that every label of this axis has
  bool steady = false;         // the range was a single value, and there is one tick, at it
  double single = 0.0;

  // Tick k, as a multiple of the step rounded to the decimals of the axis, so that 3 * 0.1 is 0.3 and not 0.30000000000000004.
  double value(int k) const {
    if (steady) return single;
    const double scale = std::pow(10.0, decimals);
    return std::round(static_cast<double>(first_index + k) * step * scale) / scale;
  }
};

// The ticks for the range from low to high, about `target` of them. A range given backwards is turned round. A range that is a single value
// has one tick, at that value, with two decimals.
inline Ticks ticks_for(double low, double high, int target) {
  // TODO: swap low and high if they are the wrong way round. If high - low is not more than 1e-12 times the larger of 1 and the biggest magnitude,
  // the range is steady: steady = true, single = low, count = 1, decimals = 2, and return. Otherwise step = nice_step(high - low, target);
  // first_index = ceil(low / step - 1e-9); last_index = floor(high / step + 1e-9); count = last_index - first_index + 1 (not below 0);
  // decimals = ceil(-log10(step) - 1e-9), and 0 if that is negative.
  (void)low;
  (void)high;
  (void)target;
  return Ticks();
}

// The label text of a tick: fixed point with the axis's decimals. A value that rounds to zero is written 0 and not -0.
inline std::string tick_label(double value, int decimals) {
  // TODO: write value with std::fixed and std::setprecision(decimals) into a std::ostringstream. If the text is all zeros but starts with '-'
  // (it was -0.004 with two decimals: "-0.00"), take the '-' off.
  (void)value;
  (void)decimals;
  return std::string();
}

#endif  // LESSON_SOLUTION_HPP
