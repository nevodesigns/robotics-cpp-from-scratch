#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <string>
#include <string_view>
#include <vector>

// The rule these all follow: take a view, return a string. A view parameter
// copies nothing at the call site, and a string return leaves no question about
// who owns the result.

inline std::string trim(std::string_view text) {
  const std::size_t first = text.find_first_not_of(" \t\r\n");

  // A failed find answers npos, which is the largest possible size. Using it as
  // a position would read far past the end, so the check is compulsory rather
  // than defensive.
  if (first == std::string_view::npos) return "";

  const std::size_t last = text.find_last_not_of(" \t\r\n");
  return std::string(text.substr(first, last - first + 1));
}

inline bool starts_with(std::string_view text, std::string_view prefix) {
  // The length test first, because compare would read past the end of a text
  // shorter than the prefix. C++20 added this as a member function; in C++17 it
  // is two lines, and writing them shows a prefix test is only ever a length
  // check and a compare.
  return text.size() >= prefix.size() && text.compare(0, prefix.size(), prefix) == 0;
}

inline bool ends_with(std::string_view text, std::string_view suffix) {
  return text.size() >= suffix.size() &&
         text.compare(text.size() - suffix.size(), suffix.size(), suffix) == 0;
}

inline std::vector<std::string> split(std::string_view text, char separator) {
  std::vector<std::string> parts;
  if (text.empty()) return parts;

  std::size_t start = 0;
  while (true) {
    const std::size_t at = text.find(separator, start);
    if (at == std::string_view::npos) {
      parts.push_back(std::string(text.substr(start)));
      return parts;
    }

    // Two separators in a row produce an empty piece on purpose. A parser needs
    // to see that a field was blank rather than have it quietly disappear.
    parts.push_back(std::string(text.substr(start, at - start)));
    start = at + 1;
  }
}

inline std::string join(const std::vector<std::string>& parts, char separator) {
  std::string out;
  for (std::size_t i = 0; i < parts.size(); ++i) {
    if (i > 0) out += separator;   // between the pieces, never after the last
    out += parts[i];
  }
  return out;
}

inline int count_char(std::string_view text, char wanted) {
  int found = 0;
  for (const char c : text) {
    if (c == wanted) ++found;
  }
  return found;
}

#endif  // LESSON_SOLUTION_HPP
