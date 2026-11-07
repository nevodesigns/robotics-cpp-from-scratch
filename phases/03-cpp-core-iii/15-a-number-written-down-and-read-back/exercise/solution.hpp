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
inline std::string shortest_text(double value) {
  // TODO: not a number is "nan", infinity is "inf" or "-inf".
  // One rule first: a whole number below 1e15 (value == std::floor(value) && std::fabs(value) < 1e15) is
  // written with std::fixed and std::setprecision(0) into a std::ostringstream, so that 100 is "100"
  // and not "1e+02".
  // Otherwise try 1 significant digit, then 2, up to 17: out << std::setprecision(digits) << value into a
  // fresh std::ostringstream, then read the text back with std::strtod(text.c_str(), nullptr). Stop at the
  // first digit count whose text reads back == value, and return that text.
  (void)value;
  return "";
}

// Read a number from text, or say it is not one. The whole of the text must be the number, and it must be
// a finite one.
inline bool parse_number(const char* text, double* out) {
  // TODO: std::strtod(text, &end). Refuse if nothing was read (end == text). Skip spaces and tabs after
  // the number; refuse if anything else is left. Refuse if the value is not finite (std::isfinite).
  // Otherwise store it in *out and return true.
  (void)text;
  (void)out;
  return false;
}

#endif  // LESSON_SOLUTION_HPP
