#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <string>

// A version of the form major.minor.patch, with an optional -prerelease and an optional +build.
struct Version {
  int major = 0;
  int minor = 0;
  int patch = 0;
  std::string pre;       // "rc.1", or empty for a release
};

namespace version_detail {

inline bool all_digits(const std::string& s) {
  if (s.empty()) return false;
  for (char c : s) {
    if (c < '0' || c > '9') return false;
  }
  return true;
}

// One number of a version: digits only, no leading zero unless it is exactly "0", and not absurdly long.
inline bool read_number(const std::string& s, int* out) {
  if (!all_digits(s) || s.size() > 9) return false;
  if (s.size() > 1 && s[0] == '0') return false;
  *out = std::stoi(s);
  return true;
}

// Compare two numbers written as digit strings without turning them into integers, so a very long
// one cannot overflow: a longer number is bigger, and equal lengths compare as text.
inline int compare_digits(const std::string& a, const std::string& b) {
  if (a.size() != b.size()) return a.size() < b.size() ? -1 : 1;
  const int c = a.compare(b);
  return c < 0 ? -1 : (c > 0 ? 1 : 0);
}

}  // namespace version_detail

// Read "1.10.2", "1.0.0-rc.1" or "2.3.4+build7". The build part after + is allowed and ignored. Anything
// else is refused and leaves *out as it was: "1.2", "1.2.3.4", "1.x.0", "01.2.3", "-1.0.0", "1.0.0-".
inline bool parse_version(const char* text, Version* out) {
  // TODO: copy the text into a std::string. Cut it at the first '+' and ignore the rest. Cut off the
  // part after the first '-' as v.pre (refuse if it is empty). What is left must be exactly three numbers
  // separated by two dots: find the dots with s.find('.'), refuse if there are not exactly two. Each
  // number goes through version_detail::read_number, which refuses non-digits, leading zeros and
  // numbers that are too long. Only when everything is right, *out = v and return true.
  (void)text;
  (void)out;
  return false;
}

// -1 if a is older than b, 0 if they are the same version, 1 if a is newer.
inline int compare(const Version& a, const Version& b) {
  // TODO: major, then minor, then patch, each compared as numbers. If those are all equal:
  //  - two releases (empty pre) are equal;
  //  - a release is NEWER than a pre-release of the same numbers, so 1.0.0-rc.1 < 1.0.0;
  //  - two pre-releases are split at the dots and compared identifier by identifier: two numbers
  //    compare as numbers (version_detail::compare_digits), a number is lower than a word, two words
  //    compare as text; if all identifiers so far are equal, the one with fewer is older.
  (void)a;
  (void)b;
  return 0;
}

// Does the version we have satisfy the version somebody asked for?
inline bool compatible(const Version& wanted, const Version& have) {
  // TODO: the major numbers must be equal. A pre-release (non-empty have.pre) is refused when wanted
  // is a release (empty wanted.pre). If wanted.major is 0 the minor numbers must be equal as
  // well. And compare(have, wanted) must not be negative: we must be at least as new.
  (void)wanted;
  (void)have;
  return false;
}

#endif  // LESSON_SOLUTION_HPP
