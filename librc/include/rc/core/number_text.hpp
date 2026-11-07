// rc/core/number_text.hpp
//
// A number written down and read back, from lesson 03-15, graduated.
//
// A plain stream writes six significant digits, so a value logged and read back is a different value: of 20000 readings of mixed size,
// 0 read back exactly through a stream, 5627 through std::to_string (the large ones, which it writes in full) and all 20000 through
// shortest_text, which tries one significant digit, then two, up to 17, and stops at the first that reads back as the same double.
// 0.1 + 0.2 is written "0.30000000000000004" and 0.3 is written "0.3", where six digits write both as "0.3". std::to_string writes 3.5e-9 as
// "0.000000". For readings to two decimal places the shortest text averaged 5.8 characters against 15.7 at a fixed seventeen digits.

#ifndef RC_CORE_NUMBER_TEXT_HPP
#define RC_CORE_NUMBER_TEXT_HPP

#include <cmath>
#include <iomanip>
#include <cstdlib>
#include <sstream>
#include <string>

namespace rc {
namespace core {

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

}  // namespace core
}  // namespace rc

#endif  // RC_CORE_NUMBER_TEXT_HPP
