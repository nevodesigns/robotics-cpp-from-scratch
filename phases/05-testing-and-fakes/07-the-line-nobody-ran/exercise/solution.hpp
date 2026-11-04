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
// TODO 1: read the report.
//
// One line at a time. Split each on its first two colons: the count field,
// the line number, then the source, which you can ignore.
//
// Skip lines numbered 0, which are gcov's own header naming the source and
// the data files. That is why the line number has to be parsed and not just
// the count field.
//
// Then decide what the count field says, and all five forms matter:
//
//   -        not code. Do not count it at all.
//   #####    code, never ran. Count it as executable and as never_run.
//   =====    the same. Counting only ##### is the mistake that reported 5
//            where the answer was 7.
//   digits*  code that ran, but not in every instance. Count it as
//            executable, and as partly_run. Leaving this out of executable
//            is the mistake that reported 56 lines where gcov said 64.
//   digits   code that ran. Count it as executable.
//
// Set ok once any line parses as count:number:, so that text which is not a
// gcov report at all can be told from a report about a file with no code in
// it. Those are different facts and a tool that confused them would report
// perfect coverage of a file it had failed to read.
inline LineCoverage parse_gcov(std::string_view text) {
  (void)text;
  return LineCoverage();
}

// The percentage gcov itself reports, to the same definition: of the lines
// that are code, the ones that ran at least once.
//
// A file with no code in it is 100 percent covered, which is true and useless,
// and is why the executable count is worth printing next to the percentage.
// TODO 2: the percentage, to gcov's own definition.
//
// Of the lines that are code, the ones that ran at least once. Mind the
// division: an integer divided by an integer is lesson 00-04's trap, and
// here it would report 0 percent for everything short of perfect.
//
// Decide what a file with no code at all should report, and be ready to
// defend it. gcov says nothing useful about such a file, and this returns
// 100, which is true and useless, and is why the executable count is worth
// printing beside the percentage.
inline double percent_covered(const LineCoverage& coverage) {
  (void)coverage;
  return 0.0;
}

#endif  // LESSON_SOLUTION_HPP
