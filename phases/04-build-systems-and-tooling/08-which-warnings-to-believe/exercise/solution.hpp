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
// TODO 1: is this line a warning, and where does its site end?
//
// This is the keystone: everything else counts what this accepts. Return the
// length of the "file:line:col" prefix, or 0 if the line is not a warning.
//
// Find ": warning: ". Then walk backwards over exactly two colon separated
// fields and insist both are all digits, which is the column and the line.
// Insist something remains before them, which is the file name.
//
// Walking backwards rather than forwards is the part that matters: a Windows
// path begins "C:\\src\\a.cpp", so the first colon is the drive letter and
// anything keyed on it is wrong on one of the four toolchains this
// curriculum claims.
//
// Returning the length rather than a bool is what lets the caller compare
// sites without allocating.
inline std::size_t warning_site_length(std::string_view line) {
  (void)line;
  return 0;
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
// TODO 2: the flag, exactly as the compiler wrote it.
//
// Including the leading dash, so "-Wsign-compare" and not "Wsign-compare".
// Empty when the warning carries no bracket, which happens: some warnings
// cannot be switched off and so have no flag to name.
//
// Be careful with the arithmetic. The first draft of this header documented
// the stripped form and returned the dashed one, and the suite is what
// noticed, which is the right way round for that to happen.
inline std::string_view warning_flag(std::string_view line) {
  (void)line;
  return std::string_view();
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
  // TODO 3: count the lines, and count the distinct sites.
  //
  // A line at a time. Use warning_site_length to decide whether it counts,
  // and compare its site against the sites of the warning lines already seen
  // to decide whether it is a new one.
  //
  // Set ok once any non empty line is seen, so an empty log is a different
  // answer from a log with no warnings in it.
  (void)log;
  return WarningCount();
}

// How many warning lines carry a given flag, named as the compiler writes
// it, as in "-Wsign-compare".
inline int count_with_flag(std::string_view log, std::string_view flag) {
  // TODO 4: how many warning lines carry this flag.
  //
  // In terms of the two functions above, so that a line which is not a
  // warning cannot contribute even if the flag text appears in it. A note
  // mentioning -Wsign-compare is not a -Wsign-compare warning.
  (void)log;
  (void)flag;
  return 0;
}

#endif  // LESSON_SOLUTION_HPP
