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
inline ParsedNumber parse_number(std::string_view field) {
  const std::string text = rc::core::trim(field);
  if (text.empty()) return ParsedNumber{};

  const char* begin = text.c_str();
  char* end = nullptr;
  errno = 0;
  const double value = std::strtod(begin, &end);

  // Anything left over means the field was not a number, it merely began with
  // one. This one line is the lesson.
  if (end != begin + text.size()) return ParsedNumber{};

  // Too large or too small for a double to hold. strtod reports this and
  // almost nobody checks it.
  if (errno == ERANGE) return ParsedNumber{};

  // nan and inf are spellings strtod accepts. A sensor cannot read either, and
  // one nan in a log poisons every average computed from it, which lesson
  // 01-08 is about. Refusing them here is cheaper than finding them later.
  if (!std::isfinite(value)) return ParsedNumber{};

  ParsedNumber parsed;
  parsed.ok = true;
  parsed.value = value;
  return parsed;
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

inline ParsedRow parse_row(std::string_view line, char separator) {
  ParsedRow row;
  const std::vector<std::string> fields = rc::core::split(line, separator);

  for (std::size_t i = 0; i < fields.size(); ++i) {
    const ParsedNumber parsed = parse_number(fields[i]);
    if (!parsed.ok) {
      row.bad_field = static_cast<int>(i);
      return row;
    }
    row.values.push_back(parsed.value);
  }

  row.ok = !row.values.empty();
  return row;
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
inline LogReport read_log(std::istream& input, char separator) {
  LogReport report;
  std::string line;

  while (std::getline(input, line)) {
    ++report.lines_read;

    // A file written on Windows and read here keeps a carriage return on the
    // end of every line. trim takes it off, along with any other whitespace,
    // which is why the trimming happens before anything looks at the content.
    const std::string cleaned = rc::core::trim(line);

    if (cleaned.empty() || cleaned[0] == '#') {
      ++report.skipped;
      continue;
    }

    const ParsedRow row = parse_row(cleaned, separator);
    if (!row.ok) {
      ++report.unparsable;
      if (report.first_bad_line == 0) report.first_bad_line = report.lines_read;
      continue;
    }

    // The first usable row decides how wide the file is. Taking it from the
    // data rather than from an argument means the caller does not have to know
    // in advance, and a file that is consistently four columns is read as four
    // columns rather than refused.
    if (report.width == 0) report.width = row.values.size();

    if (row.values.size() != report.width) {
      ++report.wrong_width;
      if (report.first_bad_line == 0) report.first_bad_line = report.lines_read;
      continue;
    }

    report.rows.push_back(row.values);
  }

  return report;
}

#endif  // LESSON_SOLUTION_HPP
