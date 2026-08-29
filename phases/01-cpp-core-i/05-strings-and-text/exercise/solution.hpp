#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <string>
#include <string_view>
#include <vector>

// Every parameter that only reads text takes a view, and every return that
// hands text back is a string, because the caller has to own it.

// Removes spaces, tabs and line endings from both ends.
inline std::string trim(std::string_view text) {
  // TODO: find_first_not_of and find_last_not_of. A failed find answers npos,
  // which is the largest possible size, so check it before using it as a
  // position. Text that is entirely whitespace trims to nothing.
  (void)text;
  return "";
}

inline bool starts_with(std::string_view text, std::string_view prefix) {
  // TODO: check the length before comparing, or a text shorter than the prefix
  // reads past its own end.
  (void)text;
  (void)prefix;
  return false;
}

inline bool ends_with(std::string_view text, std::string_view suffix) {
  // TODO
  (void)text;
  (void)suffix;
  return false;
}

// Cuts on a character. Two separators in a row produce an empty piece, which a
// parser needs to see rather than have quietly disappear.
inline std::vector<std::string> split(std::string_view text, char separator) {
  // TODO
  (void)text;
  (void)separator;
  return {};
}

// Puts the pieces back with a separator between them, and none after the last.
inline std::string join(const std::vector<std::string>& parts, char separator) {
  // TODO
  (void)parts;
  (void)separator;
  return "";
}

inline int count_char(std::string_view text, char wanted) {
  // TODO
  (void)text;
  (void)wanted;
  return 0;
}

#endif  // LESSON_SOLUTION_HPP
