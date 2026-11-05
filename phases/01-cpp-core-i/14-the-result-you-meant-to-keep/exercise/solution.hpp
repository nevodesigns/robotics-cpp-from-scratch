#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <limits>
#include <sstream>
#include <string>
#include <system_error>

// Lesson 01-12 read a file somebody else wrote. This is the other direction:
// the file your own program leaves behind, a calibration, a run's poses, the
// only record that a test drive happened.
//
// There are three ways to lose it, and none of them raises an error.
//
// It was never opened. A stream on a folder that does not exist throws
// nothing, reports nothing, and drops every write that follows. Measured:
// is_open false, good false after the first write, no exception.
//
// It was emptied by opening it. Opening a file for writing truncates it at
// the open, not at the first write. A 27 byte calibration file was 0 bytes
// the moment the stream existed, before a single byte of the new contents had
// been produced. If the program dies after that, there is no old file and no
// new one.
//
// It was lost in the buffer. Experiment: a program writes 1000 rows of seven
// bytes each, 7000 bytes, through a stream, then calls abort(). The file on
// disk was 0 bytes, and the old file it had replaced was gone too. The same
// program writing std::endl after each row left 7000 of 7000. The rows were
// never on the disk: they were in the stream's buffer, which dies with the
// process.
//
// A fourth, quieter: the disk is full. Experiment, writing to /dev/full on
// Linux: good() after the write was true, after flush it was false. The error
// arrives when the buffer is emptied, not when you wrote, so a program that
// never looks after the last write never learns.
//
// And the number itself. Of 200 computed values, running sums of 0.1 and square
// roots, a stream's default six digits read back identical for 21. digits10 is
// 15, a decimal that long survives a double; max_digits10 is 17, a double needs
// that many to survive a decimal.
//
// The cure for the first three is the same: do not touch the file until the
// new one is complete. Write beside it, check the write worked, and rename.
// Renaming over an existing file is one step on Linux and on Windows, so a
// reader sees the old file or the new one and never half of either.
//
// What this does not do: survive the power going. The rename is atomic as far
// as other programs are concerned, but the data may still be in the operating
// system's cache, and C++17 has no portable way to ask for it to be forced to
// the disk. That needs fsync, which is platform code and out of scope here.
// This protects against your program dying, which is the usual case, and not
// against the machine dying, which is a different lesson.

// What a save did, and if it did not work, why. A bool alone would be the
// silent failure this lesson is about; a message alone would have to be
// searched for to find out whether it was an error.
struct SaveResult {
  bool ok = false;
  std::string error;
};

// A double as text that reads back as the same double, as short as it can be.
//
// Starts at 15 digits, which any decimal a person would type survives, and
// widens only if the value does not come back. The result for 0.1 is "0.1"
// and for 0.1 + 0.2 is "0.30000000000000004", which is the honest answer.
//
// Not a number and the infinities are written as the words nan, inf and -inf.
// Lesson 01-12's reader refuses all three and counts them, which is better
// than a number that looks real.
// TODO 1: the shortest text that reads back as the same double.
//
// Handle the two cases that are not numbers first: std::isnan gives "nan", and
// std::isinf gives "inf" or "-inf" by sign.
//
// Then try 15 digits, which is std::numeric_limits<double>::digits10, by
// streaming the value into a std::ostringstream after calling .precision(15)
// on it. Read the text back with std::strtod(text.c_str(), nullptr) and
// compare with ==. If it matches, return it. If not, try 16, then 17, which is
// max_digits10 and always works.
//
// Do not start at 17. It is always correct and 0.1 comes out as
// 0.10000000000000001, which nobody can read.
//
// The stub returns an empty string, which cannot be mistaken for an answer.
inline std::string exact_text(double value) {
  (void)value;
  return "";
}

// TODO 2: write text to destination so it is all there or the old file is
// untouched.
//
//   1. Name a temporary file beside the destination: the same path with
//      ".tmp" added. Beside, because renaming across drives is a copy.
//   2. Open it with std::ofstream and std::ios::binary. If it did not open,
//      set result.error to a sentence that names the destination, and return.
//   3. Write the text, then CLOSE the stream and check out.fail(). Closing
//      empties the buffer, and that is when a full disk is reported.
//      If it failed, remove the temporary file and say the old file was not
//      touched.
//   4. std::filesystem::rename(temporary, destination, code) with a
//      std::error_code, so a failure is a value and not an exception. If
//      code is set, remove the temporary file and report code.message().
//   5. Otherwise result.ok = true.
//
// The stub reports failure, which is the safe direction to be wrong in: it
// never touches a file.
inline SaveResult save_text(const std::filesystem::path& destination, const std::string& text) {
  (void)destination;
  (void)text;
  SaveResult result;
  result.error = "save_text is not written yet";
  return result;
}

#endif  // LESSON_SOLUTION_HPP
