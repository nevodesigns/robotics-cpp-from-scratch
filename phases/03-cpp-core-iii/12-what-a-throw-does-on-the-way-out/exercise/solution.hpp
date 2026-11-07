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
  // TODO: strtol with errno cleared first and an end pointer.
  //  - Nothing was read (end == text) or something is left over (*end != '\0'):
  //    throw std::invalid_argument(std::string("not a number: ") + text).
  //  - errno == ERANGE or the value is above 2147483647: throw std::out_of_range("too big: " + text).
  //  - The value is zero or less: throw std::out_of_range("not positive: " + text).
  // Otherwise return the value as an int.
  (void)text;
  return 0;
}

// The same, as a function that does not throw: false and a reason instead.
inline bool try_parse_positive(const char* text, int* out, std::string* why) {
  // TODO: call parse_positive inside a try. Two handlers, most specific first, BOTH taking their
  // argument by const reference: invalid_argument sets *why to "bad input: " + e.what(), and
  // out_of_range sets it to "bad range: " + e.what(). Return true if it worked, false after a handler.
  (void)text;
  (void)out;
  (void)why;
  return true;
}

// Parse a list. If one item is bad, say which, and let the original error carry on.
inline void parse_all(const char* const* texts, int count, int* out, int* failed_index) {
  // TODO: *failed_index = -1 first. For each i, out[i] = parse_positive(texts[i]) inside a try.
  // catch (...) sets *failed_index = i and then rethrows with a bare `throw;` -- not `throw e;`,
  // which would throw a copy cut down to the base class.
  (void)texts;
  (void)count;
  (void)out;
  *failed_index = -1;
}

#endif  // LESSON_SOLUTION_HPP
