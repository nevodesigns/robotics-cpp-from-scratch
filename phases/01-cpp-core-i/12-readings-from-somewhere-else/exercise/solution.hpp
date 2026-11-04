#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cctype>
#include <cerrno>
#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <istream>
#include <string>
#include <string_view>
#include <vector>

#include <rc/core/text.hpp>

// Everything you have written so far computed its own numbers. A real robot
// project starts the other way round: somebody hands you a file off the bench
// and asks what it says.
//
// That file was written by a different program, probably on a different
// machine, possibly in a different country. It has blank lines in it, a
// comment at the top somebody added by hand, Windows line endings, one row
// that was cut short when the logger was killed, and a reading that came out
// as nan.
//
// The work is not the reading. It is deciding what to do with each line, and
// saying so, rather than quietly producing a number.

// One field, parsed.
//
// ok is false rather than the value being a sentinel, because every sentinel
// anybody picks is a reading some sensor can legitimately produce.
struct ParsedNumber {
  bool ok = false;
  double value = 0.0;
};

// A number, if the whole field is one.
//
// This is the entire difference between this and std::stod, and it is the
// reason the function exists. stod answers whether a field *starts with* a
// number. Measured: "1,5" gives 1, "1.5abc" gives 1.5, and "1.5.7" gives 1.5,
// none of them complaining. A log written where the decimal separator is a
// comma therefore reads as whole numbers, silently, and every average computed
// from it is wrong by a believable amount.
//
// So the test is not whether a number came out. It is whether the whole field
// was used.
// TODO 1: a number, if the whole field is one.
//
// Trim it first, with rc::core::trim from lesson 01-05. That takes off the
// carriage return a Windows written file leaves on every line, as well as any
// spaces, which is why it happens before anything else looks at the field.
//
// Then std::strtod, which takes a pointer to the text and a pointer to a char*
// it will set to just past whatever it consumed. Unlike std::stod it does not
// throw, and that end pointer is the whole reason to prefer it here.
//
// Four things make a field not a number:
//
//   nothing left after trimming
//   the end pointer is not at the end of the text, so something was left over
//   errno is ERANGE, so it did not fit in a double
//   the value is not finite, because nan and inf are spellings strtod accepts
//     and a sensor cannot read either
//
// Set errno to 0 before calling, because strtod only ever sets it.
//
// The test measures what std::stod does with the same fields and expects this
// to accept two of the seven where stod accepts all of them.
inline ParsedNumber parse_number(std::string_view field) {
  (void)field;
  return ParsedNumber{};
}

// One line, split and parsed.
//
// bad_field is the position of the first field that was not a number, counting
// from zero, or -1 when every field was fine. Reporting which one is the
// difference between a message somebody can act on and one they cannot.
struct ParsedRow {
  bool ok = false;
  int bad_field = -1;
  std::vector<double> values;
};

// TODO 2: split the line and parse every field.
//
// rc::core::split from lesson 01-05 does the splitting. Stop at the first
// field that is not a number and record which one it was, counting from zero,
// because "a line was bad" is not something anybody can act on.
//
// A line with no fields in it is not a row of nothing, so an empty result is
// not ok.
//
// Notice what this cannot do. It does not know how wide the file is, so a row
// cut short parses perfectly and so does one with an extra column. Those are
// caught in TODO 3 and not here.
inline ParsedRow parse_row(std::string_view line, char separator) {
  (void)line;
  (void)separator;
  return ParsedRow{};
}

// What came out of a whole log, and what did not.
//
// The counts are the point. A reader that returns only the rows it liked
// cannot be distinguished from one that found nothing wrong, and the
// difference between a log with two bad lines and a log with two thousand is
// the difference between a glitch and a format mismatch.
//
// Two separate rejection counts, because they catch different things and
// writing this taught me that the first one alone is not enough.
//
// unparsable is a field that was not a number, which is the nan from a sensor
// that dropped out.
//
// wrong_width is a row that parsed perfectly and has the wrong number of
// columns. Both of the nastiest lines in a real log land here rather than in
// unparsable. A row cut short when the logger was killed parses as a valid
// shorter row. And a reading written where the decimal separator is a comma,
// in a comma separated file, does not fail to parse at all: "0.4,1,70,2.45"
// splits into four fields and every one of them is a number. The only thing
// wrong with it is that it is four fields wide and the file is three.
struct LogReport {
  std::vector<std::vector<double>> rows;
  int lines_read = 0;
  int skipped = 0;         // blank, or a comment
  int unparsable = 0;      // a field was not a number
  int wrong_width = 0;     // parsed, and not the width of the file
  std::size_t width = 0;   // taken from the first usable row
  int first_bad_line = 0;  // 1 based, or 0 when nothing was rejected
};

// Reads every line, keeps the ones it can use, and counts the rest.
//
// Takes a stream rather than a filename so that a test can hand it a string
// and a program can hand it a file. That is the same idea as the replay sensor
// in lesson 05-02: depend on the interface, not on the device.
// TODO 3: read the whole log, keeping what you can and counting the rest.
//
// std::getline in a while loop reads it a line at a time. For each line:
//
//   count it
//   trim it
//   skip it if it is empty or starts with #
//   parse it, and count it as unparsable if a field was not a number
//   take the width from the first usable row
//   count it as wrong_width if it does not match, and keep it otherwise
//
// Record first_bad_line the first time anything is rejected, one based and
// counting every line in the file including the skipped ones, because that is
// what an editor shows.
//
// Taking the width from the data rather than from an argument means a caller
// does not have to know in advance, and a file that is consistently four
// columns is read as four columns rather than refused.
//
// The test gives it a log with three bad lines in it and expects one of them
// to be unparsable and two to be the wrong width.
inline LogReport read_log(std::istream& input, char separator) {
  (void)input;
  (void)separator;
  return LogReport{};
}

#endif  // LESSON_SOLUTION_HPP
