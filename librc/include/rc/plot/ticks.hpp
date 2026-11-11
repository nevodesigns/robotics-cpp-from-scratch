// rc/plot/ticks.hpp
//
// Axis ticks on nice numbers, from lesson 10-05, graduated.
//
// Added ten times, 0.1 is 0.9999999999999999, and 3 * 0.1 prints as 0.30000000000000004. Every tick here is a whole number times a step of 1, 2 or 5 times a power of
// ten, rounded to the decimals the step needs, and a label that rounds to zero loses its minus sign.

#ifndef RC_PLOT_TICKS_HPP
#define RC_PLOT_TICKS_HPP

#include <cmath>
#include <iomanip>
#include <sstream>
#include <string>

namespace rc {
namespace plot {

// An axis needs numbers written on it, and a person wants them at places that are easy to read: 0, 0.2, 0.4 and not 0.137, 0.274. The step
// between them is 1, 2 or 5 times a power of ten, and the first tick is the first multiple of the step that is inside the range.
//
// Every tick is computed as a whole number times the step, and not by adding the step to the last tick, because adding 0.1 ten times is not 1.

// The step between ticks: 1, 2, 5 or 10 times a power of ten, the smallest of those that is at least span / target. A span or a target that
// is not positive gives 1.
inline double nice_step(double span, int target) {
  if (!(span > 0.0) || target < 1) return 1.0;
  const double raw = span / static_cast<double>(target);
  const double magnitude = std::pow(10.0, std::floor(std::log10(raw)));
  const double fraction = raw / magnitude;
  double nice = 10.0;
  if (fraction <= 1.0 + 1e-9) nice = 1.0;
  else if (fraction <= 2.0 + 1e-9) nice = 2.0;
  else if (fraction <= 5.0 + 1e-9) nice = 5.0;
  return nice * magnitude;
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
  Ticks t;
  if (high < low) {
    const double swap = low;
    low = high;
    high = swap;
  }
  const double span = high - low;
  const double size = std::fabs(low) > std::fabs(high) ? std::fabs(low) : std::fabs(high);
  if (!(span > 1e-12 * (size > 1.0 ? size : 1.0))) {
    t.steady = true;
    t.single = low;
    t.count = 1;
    t.decimals = 2;
    return t;
  }
  t.step = nice_step(span, target);
  t.first_index = static_cast<long long>(std::ceil(low / t.step - 1e-9));
  const long long last_index = static_cast<long long>(std::floor(high / t.step + 1e-9));
  t.count = static_cast<int>(last_index - t.first_index + 1);
  if (t.count < 0) t.count = 0;
  const double places = std::ceil(-std::log10(t.step) - 1e-9);
  t.decimals = places > 0.0 ? static_cast<int>(places) : 0;
  return t;
}

// The label text of a tick: fixed point with the axis's decimals. A value that rounds to zero is written 0 and not -0.
inline std::string tick_label(double value, int decimals) {
  std::ostringstream out;
  out << std::fixed << std::setprecision(decimals) << value;
  std::string text = out.str();
  bool all_zero = true;
  for (char c : text)
    if (c != '-' && c != '0' && c != '.') all_zero = false;
  if (all_zero && !text.empty() && text[0] == '-') text.erase(0, 1);
  return text;
}
}  // namespace plot
}  // namespace rc

#endif  // RC_PLOT_TICKS_HPP
