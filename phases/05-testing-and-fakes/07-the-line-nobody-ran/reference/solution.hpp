#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cstddef>
#include <string_view>

// Lesson 04-07 ended on a sentence that was left hanging: a sanitizer only
// ever reports what the test actually ran, so the lane is worth exactly as
// much as the suite's coverage. This lesson is about finding out what that is.
//
// The tool is gcov, which ships with gcc, so there is nothing to install. The
// compiler is asked for --coverage, which makes it count how many times each
// line ran, the program is run, and gcov turns the counts into an annotated
// copy of the source.
//
// A line of that output looks like this, count first, then line number, then
// the source:
//
//         -:    0:Source:/path/to/solution.hpp
//         -:   12:// a comment, which is not code
//         1:   20:  return value;
//        1*:   27:  table[i] = compute(i);
//     #####:  159:      rest.push_back(args_[i]);
//     =====:   63:  }
//
// Five forms, and getting them wrong is the first thing that happens.
//
//   -        the line is not code: a comment, a blank, a declaration
//   digits   it ran that many times
//   digits*  it ran, but not every instance of it did, which happens with
//            templates and inlining
//   #####    it is code and it never ran
//   =====    it is code and it never ran, because the block holding it was
//            never entered
//
// Both #####, and ===== mean not covered. The author of this lesson counted
// only the first and reported 5 never-run lines where the answer was 7, then
// counted both and still disagreed with gcov about the total, because the
// digits* form was not being counted as code at all. Three attempts, and the
// third agreed with gcov exactly.
//
// That is the argument for this header. The format is simple enough to parse
// with a glance and wrong enough to punish one.

// What a gcov report says about one file.
struct LineCoverage {
  int executable = 0;   // lines gcov considers code
  int never_run = 0;    // of those, the ones with no count
  int partly_run = 0;   // of those, the ones in the digits* form
  bool ok = false;      // the text looked like a gcov report at all
};

// Reads the annotated source gcov produces.
//
// Lines numbered 0 are gcov's own header, naming the source file, the data
// files and the number of runs. They are skipped rather than counted, which
// is why the line number has to be parsed and not just the count.
inline LineCoverage parse_gcov(std::string_view text) {
  LineCoverage result;
  std::size_t start = 0;
  while (start <= text.size()) {
    const std::size_t newline = text.find('\n', start);
    const std::size_t end = newline == std::string_view::npos ? text.size() : newline;
    const std::string_view line = text.substr(start, end - start);
    start = end + 1;
    if (newline == std::string_view::npos) {
      if (line.empty()) break;
    }

    // count field, then ':', then line number, then ':'
    const std::size_t first_colon = line.find(':');
    if (first_colon == std::string_view::npos) continue;
    const std::size_t second_colon = line.find(':', first_colon + 1);
    if (second_colon == std::string_view::npos) continue;

    std::string_view count = line.substr(0, first_colon);
    std::string_view number = line.substr(first_colon + 1, second_colon - first_colon - 1);
    while (!count.empty() && count.front() == ' ') count.remove_prefix(1);
    while (!number.empty() && number.front() == ' ') number.remove_prefix(1);
    if (count.empty() || number.empty()) continue;

    // A line number of zero is gcov's own header, not source.
    bool numeric = true;
    long value = 0;
    for (const char c : number) {
      if (c < '0' || c > '9') { numeric = false; break; }
      value = value * 10 + (c - '0');
    }
    if (!numeric) continue;
    result.ok = true;
    if (value == 0) continue;

    if (count == "-") continue;                       // not code
    if (count == "#####" || count == "=====") {       // code, never ran
      ++result.executable;
      ++result.never_run;
      continue;
    }
    bool starred = false;
    if (count.back() == '*') { starred = true; count.remove_suffix(1); }
    bool digits = !count.empty();
    for (const char c : count)
      if (c < '0' || c > '9') { digits = false; break; }
    if (!digits) continue;
    ++result.executable;
    if (starred) ++result.partly_run;
  }
  return result;
}

// The percentage gcov itself reports, to the same definition: of the lines
// that are code, the ones that ran at least once.
//
// A file with no code in it is 100 percent covered, which is true and useless,
// and is why the executable count is worth printing next to the percentage.
inline double percent_covered(const LineCoverage& coverage) {
  if (coverage.executable == 0) return 100.0;
  const int ran = coverage.executable - coverage.never_run;
  return 100.0 * static_cast<double>(ran) / static_cast<double>(coverage.executable);
}

#endif  // LESSON_SOLUTION_HPP
