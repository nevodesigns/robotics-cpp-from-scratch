#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cerrno>
#include <cstdlib>
#include <stdexcept>
#include <string>

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

#endif  // LESSON_SOLUTION_HPP
