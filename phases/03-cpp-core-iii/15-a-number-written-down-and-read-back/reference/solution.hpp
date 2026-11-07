#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cmath>
#include <iomanip>
#include <cstdlib>
#include <sstream>
#include <string>

// What a plain stream gives: six significant digits, which is what `std::cout << x` does. Given.
inline std::string default_text(double value) {
  std::ostringstream out;
  out << value;
  return out.str();
}

// The shortest text that reads back as exactly this double.
//
// A double has about 16 decimal digits of precision, and up to 17 are needed to tell two neighbouring
// doubles apart. Most numbers need fewer: 0.5 is "0.5". So try one digit, then two, and stop at the
// first that reads back as the same number.
//
// Not a number and infinity have no digits to try, and are written out as words.
inline std::string shortest_text(double value) {
  if (std::isnan(value)) return "nan";
  if (std::isinf(value)) return value > 0.0 ? "inf" : "-inf";

  // A whole number below 1e15 is written as itself. Plain significant-digit output with one digit would
  // write 100 as "1e+02", which reads back correctly and is not what a person wants to see.
  if (value == std::floor(value) && std::fabs(value) < 1e15) {
    std::ostringstream whole;
    whole << std::fixed << std::setprecision(0) << value;
    return whole.str();
  }

  std::string text;
  for (int digits = 1; digits <= 17; ++digits) {
    std::ostringstream out;
    out << std::setprecision(digits) << value;
    text = out.str();
    if (std::strtod(text.c_str(), nullptr) == value) break;
  }
  return text;
}

// Read a number from text, or say it is not one. The whole of the text must be the number, and it must be
// a finite one: "1.5x" is refused, so is "", so is "nan", and so is "inf", which a configuration file
// never means.
inline bool parse_number(const char* text, double* out) {
  char* end = nullptr;
  const double value = std::strtod(text, &end);
  if (end == text) return false;
  while (*end == ' ' || *end == '\t') ++end;
  if (*end != '\0') return false;
  if (!std::isfinite(value)) return false;
  *out = value;
  return true;
}

#endif  // LESSON_SOLUTION_HPP
