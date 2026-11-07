// rc/core/checked_int.hpp
//
// Parsing with errors that are told apart, from lesson 03-12, graduated.
//
// A parse that throws two different types, std::invalid_argument for text that is not a number and std::out_of_range for one that is no use,
// lets a caller handle them differently. Catching by reference keeps what makes the error specific: the same invalid_argument caught by value
// was no longer an invalid_argument on any toolchain, and on libstdc++ its message became "std::exception" too; by reference it kept both. Rethrowing with a bare `throw;` kept the type, where `throw e;` cut it down to
// std::exception. On the way out of a throw, destructors ran in reverse order (drop C, drop B, drop A) before the handler did.

#ifndef RC_CORE_CHECKED_INT_HPP
#define RC_CORE_CHECKED_INT_HPP

#include <cerrno>
#include <cstdlib>
#include <stdexcept>
#include <string>

namespace rc {
namespace core {

// Turn text into a whole number above zero, or throw.
//
// Two different things can be wrong and they are different types, so a caller can tell them apart:
// text that is not a number at all is std::invalid_argument, and a number that is no use here, zero,
// negative or too big to hold, is std::out_of_range. Both carry the offending text in their message.
inline int parse_positive(const char* text) {
  char* end = nullptr;
  errno = 0;
  const long value = std::strtol(text, &end, 10);
  if (end == text || *end != '\0') throw std::invalid_argument(std::string("not a number: ") + text);
  if (errno == ERANGE || value > 2147483647L) throw std::out_of_range(std::string("too big: ") + text);
  if (value <= 0) throw std::out_of_range(std::string("not positive: ") + text);
  return static_cast<int>(value);
}

// The same, as a function that does not throw: false and a reason instead.
//
// The two handlers are written most specific first and take their argument by const reference. By
// value would copy the exception into a std::exception and cut off everything that made it
// special, including its message.
inline bool try_parse_positive(const char* text, int* out, std::string* why) {
  try {
    *out = parse_positive(text);
    return true;
  } catch (const std::invalid_argument& e) {
    *why = std::string("bad input: ") + e.what();
  } catch (const std::out_of_range& e) {
    *why = std::string("bad range: ") + e.what();
  }
  return false;
}

// Parse a list. If one item is bad, say which, and let the original error carry on.
//
// `throw;` with nothing after it rethrows the very object that was caught, with its own type. Writing
// `throw e;` would throw a copy made from the type of the variable, which is the base class.
inline void parse_all(const char* const* texts, int count, int* out, int* failed_index) {
  *failed_index = -1;
  for (int i = 0; i < count; ++i) {
    try {
      out[i] = parse_positive(texts[i]);
    } catch (...) {
      *failed_index = i;
      throw;
    }
  }
}

}  // namespace core
}  // namespace rc

#endif  // RC_CORE_CHECKED_INT_HPP
