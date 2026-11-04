// rc/test/coverage.hpp
//
// Reading a gcov report, from lesson 05-07, graduated.
//
// Lesson 04-07 ended on a sentence left hanging: a sanitizer only ever
// reports what the test actually ran, so the lane is worth exactly as much as
// the suite's coverage. This is how to find out what that is.
//
// gcov ships with gcc, so there is nothing to install. Compile with
// --coverage, run the program, and gcov writes an annotated copy of the
// source with a count against every line.
//
// The headline, which is the part people get backwards: coverage answers one
// question, did this line run, and it does not ask whether anything checked
// the result. So it is a floor, not a score. A line that never ran certainly
// is not tested. A line that ran may or may not be.
//
// Measured for the lesson: a program calling five functions and asserting
// nothing at all reported 100 percent of 5 lines with 0 never run, while a
// deliberate defect in one of them, a > where the intention was >=, survived
// untouched. And on a line of the form `return a && b`, gcov reported lines
// executed 100 percent of 2, branches executed 100 percent of 6, and taken at
// least once 50 percent of 6. Three numbers, three different stories, one run.
//
// Print the executable count beside the percentage, always. 100 percent of 0
// and 100 percent of 600 are the same number and not the same claim.
//
// On the format, which is where this header earns its place. There are five
// count forms and three of them are easy to miscount:
//
//   -        the line is not code: a comment, a blank, a declaration
//   digits   it ran that many times
//   digits*  it ran, but not every instance of it did, which happens with
//            templates and inlining
//   #####    it is code and it never ran
//   =====    it is code and it never ran, the block was never entered
//
// The author of lesson 05-07 counted this by hand three times before agreeing
// with gcov. Counting only ##### gave 5 never-run lines where the answer was
// 7. Counting both markers still disagreed on the total, 56 against gcov's
// 64, because the digits* form was not being counted as code at all. Two of
// the three wrong answers were wrong in the flattering direction.
//
// Verified against gcov on four real files: 64 executable with 7 never run,
// 15 with 0, 142 with 0, and 5 with 0. Same numbers, same percentages.

#ifndef RC_TEST_COVERAGE_HPP
#define RC_TEST_COVERAGE_HPP

#include <cstddef>
#include <string_view>

namespace rc {
namespace test {

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

}  // namespace test
}  // namespace rc

#endif  // RC_TEST_COVERAGE_HPP
