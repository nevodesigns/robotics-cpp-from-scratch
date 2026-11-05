#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cstddef>
#include <string_view>

// Lesson 00-03 was about errors, which stop the build and have to be dealt
// with. This one is about warnings, which do not, and therefore have to be
// judged.
//
// The judging is the hard part and it is not optional. A project that treats
// every warning as a defect spends days on false positives. A project that
// ignores them loses the one warning in ten that was the bug. This curriculum
// has done both, and the measurements below are its own.
//
// Start with counting, because the first number everyone quotes is wrong.

// A build log line that reports a warning.
//
// Both compilers use the same shape, which makes one parser enough:
//
//   w.cpp:3:51: warning: comparison of integer expressions ... [-Wsign-compare]
//   w.cpp:4:31: warning: missing field 'y' initializer [-Wmissing-field-initializers]
//
// What is not a warning, and must not be counted:
//
//   w.cpp: In instantiation of 'int cmp(T, std::size_t) ...'   context, no line
//   w.cpp:3:23:   required from here                            context
//       2 | template <class T> int cmp(T n, ...                  the source echo
//   3 warnings generated.                                        clang's summary
//   w.cpp:9:1: note: declared here                               a note
//
// Keying on the full file:line:col: warning: shape excludes all of them,
// which is why the column is parsed and not skipped.
struct WarningCount {
  int lines = 0;    // warning lines in the log
  int sites = 0;    // distinct file:line:col among them
  bool ok = false;  // the text looked like a build log at all
};

// Whether a line is a warning, and where its site ends.
//
// Returns the length of the "file:line:col" prefix, or 0 if this is not a
// warning line. Taking the prefix rather than a bool is what lets the caller
// count distinct sites without allocating.
inline std::size_t warning_site_length(std::string_view line) {
  const std::size_t marker = line.find(": warning: ");
  if (marker == std::string_view::npos) return 0;

  // Walk back over the column and line numbers: :<digits>:<digits>
  std::string_view prefix = line.substr(0, marker);
  int colons_with_digits = 0;
  while (colons_with_digits < 2) {
    const std::size_t colon = prefix.rfind(':');
    if (colon == std::string_view::npos) return 0;
    const std::string_view number = prefix.substr(colon + 1);
    if (number.empty()) return 0;
    for (const char c : number)
      if (c < '0' || c > '9') return 0;
    prefix = prefix.substr(0, colon);
    ++colons_with_digits;
  }
  if (prefix.empty()) return 0;   // no file name before the numbers
  return marker;
}

// The flag in brackets at the end, exactly as the compiler wrote it and so
// including its leading dash, as in "-Wsign-compare". Empty if the warning
// has none, because some cannot be switched off and carry no bracket at all.
//
// Keeping the dash is deliberate. It is the form a compiler prints, the form
// you paste into a search, and the form you write in a -Wno- flag, so
// stripping it would mean every caller put it back. The first draft of this
// header promised the stripped form in its comment and returned the dashed
// one, and the suite caught the disagreement.
inline std::string_view warning_flag(std::string_view line) {
  const std::size_t open = line.rfind(" [-W");
  if (open == std::string_view::npos) return std::string_view();
  const std::size_t close = line.rfind(']');
  if (close == std::string_view::npos || close < open) return std::string_view();
  return line.substr(open + 2, close - open - 2);
}

// Counts a whole build log.
//
// The distinction that matters: lines against sites. One defect in a header
// produces one warning per translation unit that includes it, so the line
// count scales with the size of the project and the site count does not.
//
// Measured for this lesson: one bad comparison in a header, three translation
// units, gave 3 warning lines and 1 distinct site. The same effect is why this
// repository once reported 98 of -Wclobbered and 51 of -Wmismatched-new-delete
// from a handful of defects.
inline WarningCount count_warnings(std::string_view log) {
  WarningCount result;
  std::size_t start = 0;
  // Sites are counted by comparing against the ones already seen, which needs
  // no container: the log is walked twice, and a log is small.
  while (start <= log.size()) {
    const std::size_t newline = log.find('\n', start);
    const std::size_t end = newline == std::string_view::npos ? log.size() : newline;
    const std::string_view line = log.substr(start, end - start);
    if (!line.empty()) result.ok = true;

    const std::size_t site_length = warning_site_length(line);
    if (site_length > 0) {
      ++result.lines;
      const std::string_view site = line.substr(0, site_length);

      bool seen = false;
      std::size_t scan = 0;
      while (scan < start) {
        const std::size_t scan_end_nl = log.find('\n', scan);
        const std::size_t scan_end = scan_end_nl == std::string_view::npos ? log.size() : scan_end_nl;
        const std::string_view earlier = log.substr(scan, scan_end - scan);
        const std::size_t earlier_length = warning_site_length(earlier);
        if (earlier_length == site_length && earlier.substr(0, earlier_length) == site) {
          seen = true;
          break;
        }
        scan = scan_end + 1;
      }
      if (!seen) ++result.sites;
    }

    if (newline == std::string_view::npos) break;
    start = end + 1;
  }
  return result;
}

// How many warning lines carry a given flag, named as the compiler writes
// it, as in "-Wsign-compare".
inline int count_with_flag(std::string_view log, std::string_view flag) {
  int found = 0;
  std::size_t start = 0;
  while (start <= log.size()) {
    const std::size_t newline = log.find('\n', start);
    const std::size_t end = newline == std::string_view::npos ? log.size() : newline;
    const std::string_view line = log.substr(start, end - start);
    if (warning_site_length(line) > 0 && warning_flag(line) == flag) ++found;
    if (newline == std::string_view::npos) break;
    start = end + 1;
  }
  return found;
}

#endif  // LESSON_SOLUTION_HPP
