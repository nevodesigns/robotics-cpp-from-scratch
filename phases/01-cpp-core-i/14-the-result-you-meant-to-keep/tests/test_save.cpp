#include <rc/test/rc_test.hpp>
#include <rc/core/parse.hpp>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>
#include <vector>

#include "solution.hpp"

namespace fs = std::filesystem;

// File scope, per audit rule L022.
const char kVariantDir[] = RC_LESSON_VARIANT_DIR;
const char kOldCalibration[] = "offset 0.0125\nscale 1.0042\n";  // 27 bytes
const char kNewCalibration[] = "offset 0.0131\nscale 1.0040\n";

namespace {

// A folder that exists for one test and is gone after it, even when a
// RC_REQUIRE stops the test early, because the destructor still runs.
//
// The folder is named after the variant being tested, so that the exercise and
// the reference suites, which ctest may run at the same moment, never share
// one and never delete each other's files.
class Scratch {
 public:
  Scratch() {
    const std::string variant = fs::path(kVariantDir).filename().string();
    dir_ = fs::temp_directory_path() / ("rc_01_14_" + variant);
    std::error_code ignored;
    fs::remove_all(dir_, ignored);
    fs::create_directories(dir_);
  }
  ~Scratch() {
    std::error_code ignored;
    fs::remove_all(dir_, ignored);
  }
  Scratch(const Scratch&) = delete;
  Scratch& operator=(const Scratch&) = delete;

  const fs::path& dir() const { return dir_; }
  fs::path file(const std::string& name) const { return dir_ / name; }

 private:
  fs::path dir_;
};

void write_file(const fs::path& path, const std::string& text) {
  std::ofstream out(path, std::ios::binary);
  out << text;
}

std::string read_file(const fs::path& path) {
  std::ifstream in(path, std::ios::binary);
  std::ostringstream all;
  all << in.rdbuf();
  return all.str();
}

// The size on disk, or -1 when there is no such file.
long long size_of(const fs::path& path) {
  std::error_code code;
  const auto bytes = fs::file_size(path, code);
  return code ? -1LL : static_cast<long long>(bytes);
}

int entries_in(const fs::path& dir) {
  int count = 0;
  for (const auto& entry : fs::directory_iterator(dir)) {
    (void)entry;
    ++count;
  }
  return count;
}

// The way everyone writes it first, modelling a program that stopped after
// stop_after bytes had reached the disk. Opening for writing empties the file,
// so by the time the first byte is written the old contents are already gone.
//
// This models the stop rather than causing one, because a test cannot kill
// itself and report. The lesson's own text measures the real thing, with abort.
void write_in_place(const fs::path& path, const std::string& text, std::size_t stop_after) {
  std::ofstream out(path, std::ios::binary);
  const std::size_t keep = std::min(stop_after, text.size());
  out.write(text.data(), static_cast<std::streamsize>(keep));
}

}  // namespace

RC_TEST("opening a file to write to it empties it, and a save that fails does not") {
  const Scratch scratch;
  const fs::path target = scratch.file("cal.txt");

  write_file(target, kOldCalibration);
  RC_REQUIRE_EQ(size_of(target), 27LL);

  long long just_opened = -1;
  {
    std::ofstream out(target);
    just_opened = size_of(target);
  }
  std::cout << "  a calibration file of " << 27 << " bytes, opened for writing and\n";
  std::cout << "  nothing yet written to it:           " << just_opened << " bytes\n\n";
  RC_CHECK_EQ(just_opened, 0LL);

  // The same file, and a program that stops after five bytes of the new
  // contents. Nothing of the old file is left, and what is there is not the
  // new file either.
  write_file(target, kOldCalibration);
  write_in_place(target, kNewCalibration, 5);
  std::cout << "  written in place, stopped after 5 bytes: " << size_of(target)
            << " bytes, \"" << read_file(target) << "\"\n";
  RC_CHECK_EQ(size_of(target), 5LL);

  // Now the artifact. A folder where the temporary file has to go makes the
  // save fail before it has touched the destination, which is how a disk that
  // fills up or a path that is wrong looks from outside.
  write_file(target, kOldCalibration);
  fs::create_directory(scratch.file("cal.txt.tmp"));
  const SaveResult refused = save_text(target, kNewCalibration);
  std::cout << "  saved, with the temporary file blocked:  ok=" << (refused.ok ? "yes" : "no")
            << ", the old file is " << size_of(target) << " bytes\n\n";
  RC_CHECK(!refused.ok);
  RC_CHECK(!refused.error.empty());
  RC_CHECK_EQ(read_file(target), std::string(kOldCalibration));

  // A save that refuses everything would pass every line above, so the same
  // folder, unblocked, has to work. Refusing is only worth anything from a
  // function that can also succeed.
  fs::remove(scratch.file("cal.txt.tmp"));
  const SaveResult retry = save_text(target, kNewCalibration);
  std::cout << "  unblocked, saved again:                  ok=" << (retry.ok ? "yes" : "no")
            << ", the file is now \"" << read_file(target).substr(0, 13) << "...\"\n\n";
  RC_REQUIRE(retry.ok);
  RC_CHECK_EQ(read_file(target), std::string(kNewCalibration));
}

RC_TEST("a save that works replaces the file whole and leaves nothing behind") {
  const Scratch scratch;
  const fs::path target = scratch.file("cal.txt");

  // First time: there is nothing to replace.
  const SaveResult first = save_text(target, kOldCalibration);
  RC_REQUIRE(first.ok);
  RC_CHECK_EQ(read_file(target), std::string(kOldCalibration));
  RC_CHECK_EQ(entries_in(scratch.dir()), 1);

  // Second time: the old file is replaced in one step.
  const SaveResult second = save_text(target, kNewCalibration);
  RC_REQUIRE(second.ok);
  RC_CHECK(second.error.empty());
  RC_CHECK_EQ(read_file(target), std::string(kNewCalibration));

  // The temporary file is a means and not a result, so it must not be there.
  RC_CHECK_EQ(entries_in(scratch.dir()), 1);
  RC_CHECK(!fs::exists(scratch.file("cal.txt.tmp")));

  // Saving nothing is saving an empty file, which is a different thing from
  // not saving.
  const SaveResult empty = save_text(target, "");
  RC_REQUIRE(empty.ok);
  RC_CHECK_EQ(size_of(target), 0LL);
}

RC_TEST("the bytes you give are the bytes on disk") {
  const Scratch scratch;
  const fs::path target = scratch.file("bytes.bin");

  // A newline, a Windows line ending, and a zero byte in the middle. A stream
  // opened in text mode would turn the first into two bytes on Windows, and a
  // function that took a C string would stop at the third.
  const std::string given("a\nb\r\nc\0d", 8);
  RC_REQUIRE_EQ(given.size(), 8u);

  const SaveResult saved = save_text(target, given);
  RC_REQUIRE(saved.ok);

  std::cout << "  given " << given.size() << " bytes, on disk " << size_of(target) << "\n\n";
  RC_CHECK_EQ(size_of(target), 8LL);
  RC_CHECK(read_file(target) == given);
}

RC_TEST("a path that cannot be opened is an answer, not a silent nothing") {
  const Scratch scratch;
  const fs::path missing = scratch.dir() / "no_such_folder" / "run.csv";

  // What a stream does when it cannot open a file: nothing visible. No
  // exception, no message, and every later write is quietly dropped.
  bool threw = false;
  bool opened = true;
  bool still_good = true;
  try {
    std::ofstream out(missing);
    opened = out.is_open();
    out << "0.1,1.5\n";
    out.flush();
    still_good = out.good();
  } catch (...) {
    threw = true;
  }
  std::cout << "  a stream on a folder that does not exist:\n";
  std::cout << "    threw an exception   " << (threw ? "yes" : "no") << "\n";
  std::cout << "    is_open              " << (opened ? "yes" : "no") << "\n";
  std::cout << "    good after a write   " << (still_good ? "yes" : "no") << "\n\n";
  RC_CHECK(!threw);
  RC_CHECK(!opened);
  RC_CHECK(!still_good);

  const SaveResult saved = save_text(missing, "0.1,1.5\n");
  std::cout << "  save_text on the same path: ok=" << (saved.ok ? "yes" : "no")
            << "\n    " << saved.error << "\n\n";
  RC_CHECK(!saved.ok);

  // A message the person reading it can act on names the file.
  RC_CHECK(saved.error.find("run.csv") != std::string::npos);
  RC_CHECK(!fs::exists(scratch.dir() / "no_such_folder"));
}

RC_TEST("a log that stops partway is a valid, shorter, wrong log") {
  const Scratch scratch;
  const fs::path target = scratch.file("run.csv");

  // Ten rows of eight bytes: "0.5,1.5\n".
  std::string log;
  for (int i = 0; i < 10; ++i) log += "0.5,1.5\n";
  RC_REQUIRE_EQ(log.size(), 80u);

  std::cout << "  a ten row log, written in place, and stopped after N bytes:\n\n";
  std::cout << "    stopped at   on disk   rows   unparsable   wrong width   last value\n";

  struct Outcome {
    int rows = 0;
    int unparsable = 0;
    int wrong_width = 0;
    double last = 0.0;
  };
  std::vector<std::size_t> stops = {80, 77, 13, 11, 0};
  std::vector<Outcome> outcomes;

  for (const std::size_t stop : stops) {
    write_in_place(target, log, stop);
    std::ifstream in(target, std::ios::binary);
    const rc::core::LogReport report = rc::core::read_log(in, ',');
    Outcome outcome;
    outcome.rows = static_cast<int>(report.rows.size());
    outcome.unparsable = report.unparsable;
    outcome.wrong_width = report.wrong_width;
    outcome.last = report.rows.empty() ? 0.0 : report.rows.back().back();
    outcomes.push_back(outcome);

    std::cout << "    " << std::setw(10) << stop << "   " << std::setw(7) << size_of(target) << "   "
              << std::setw(4) << outcome.rows << "   " << std::setw(10) << outcome.unparsable
              << "   " << std::setw(11) << outcome.wrong_width << "   " << std::setw(10)
              << outcome.last << "\n";
  }
  std::cout << "\n";

  // The whole log reads as ten rows of 1.5.
  RC_CHECK_EQ(outcomes[0].rows, 10);

  // Cut after "0.5,1" the last row is the right width and a perfectly good
  // number, 1, where the logger meant 1.5. Nothing was rejected, so nothing
  // was reported, and the file is simply wrong.
  RC_CHECK_EQ(outcomes[1].rows, 10);
  RC_CHECK_EQ(outcomes[1].unparsable + outcomes[1].wrong_width, 0);
  RC_CHECK_NEAR(outcomes[1].last, 1.0, 1e-12);

  // Cut earlier, the same thing at row two.
  RC_CHECK_EQ(outcomes[2].rows, 2);
  RC_CHECK_EQ(outcomes[2].unparsable + outcomes[2].wrong_width, 0);

  // Cut after "0.5" the row is one column wide, which is at least noticed.
  RC_CHECK_EQ(outcomes[3].wrong_width, 1);

  // And now the artifact: the same log, saved, is whole or absent.
  const SaveResult saved = save_text(target, log);
  RC_REQUIRE(saved.ok);
  std::ifstream whole(target, std::ios::binary);
  const rc::core::LogReport report = rc::core::read_log(whole, ',');
  RC_CHECK_EQ(report.rows.size(), 10u);
  RC_CHECK_EQ(report.unparsable + report.wrong_width, 0);
  RC_CHECK_EQ(size_of(target), 80LL);
}

RC_TEST("a number written with the stream's default settings does not read back") {
  // Values a robot actually has: not typed constants but results of
  // arithmetic. A running sum of 0.1, and square roots.
  std::vector<double> values;
  double running = 0.0;
  for (int i = 1; i <= 100; ++i) {
    running += 0.1;
    values.push_back(running);
    values.push_back(std::sqrt(static_cast<double>(i)));
  }

  int default_back = 0;
  int exact_back = 0;
  for (const double value : values) {
    std::ostringstream plain;
    plain << value;
    if (std::stod(plain.str()) == value) ++default_back;

    const std::string careful = exact_text(value);
    if (!careful.empty() && std::stod(careful) == value) ++exact_back;
  }

  const int total = static_cast<int>(values.size());
  std::cout << "  " << total << " computed values, written as text and read back:\n";
  std::cout << "    with the stream's default 6 digits   " << default_back << " came back identical\n";
  std::cout << "    with exact_text                      " << exact_back << " came back identical\n\n";
  std::cout << "  digits10 = " << std::numeric_limits<double>::digits10
            << " (a decimal this long survives a double)\n";
  std::cout << "  max_digits10 = " << std::numeric_limits<double>::max_digits10
            << " (a double this long survives a decimal)\n\n";

  RC_CHECK_EQ(exact_back, total);
  RC_CHECK(default_back < total);
  RC_CHECK_EQ(std::numeric_limits<double>::digits10, 15);
  RC_CHECK_EQ(std::numeric_limits<double>::max_digits10, 17);
}

RC_TEST("exact_text gives the short answer when there is one and the long one when there is not") {
  // Known answers first. Checking only that a value reads back would pass for
  // a function that always printed 17 digits, which is correct and unreadable.
  RC_CHECK_EQ(exact_text(0.5), std::string("0.5"));
  RC_CHECK_EQ(exact_text(0.1), std::string("0.1"));
  RC_CHECK_EQ(exact_text(2.45), std::string("2.45"));
  RC_CHECK_EQ(exact_text(100.0), std::string("100"));
  RC_CHECK_EQ(exact_text(-2.5), std::string("-2.5"));
  RC_CHECK_EQ(exact_text(0.0), std::string("0"));

  // 0.1 + 0.2 is not 0.3, and the text has to say so. 16 digits is not enough
  // here and 17 is.
  RC_CHECK_EQ(exact_text(0.1 + 0.2), std::string("0.30000000000000004"));
  RC_CHECK_EQ(exact_text(1.0 / 3.0), std::string("0.3333333333333333"));

  // Not a number and the infinities are written as words, and the reader from
  // lesson 01-12 refuses all three, so they are counted instead of invented.
  const double nan = std::numeric_limits<double>::quiet_NaN();
  const double inf = std::numeric_limits<double>::infinity();
  RC_CHECK_EQ(exact_text(nan), std::string("nan"));
  RC_CHECK_EQ(exact_text(inf), std::string("inf"));
  RC_CHECK_EQ(exact_text(-inf), std::string("-inf"));
  RC_CHECK(!rc::core::parse_number(exact_text(nan)).ok);
  RC_CHECK(!rc::core::parse_number(exact_text(inf)).ok);
}

RC_TEST("what is written with exact_text is what the 01-12 reader reads") {
  const Scratch scratch;
  const fs::path target = scratch.file("pose.csv");

  std::vector<double> xs;
  std::vector<double> ys;
  double x = 0.0;
  double y = 0.0;
  for (int i = 0; i < 50; ++i) {
    x += 0.1 * std::cos(0.05 * i);
    y += 0.1 * std::sin(0.05 * i);
    xs.push_back(x);
    ys.push_back(y);
  }
  xs.push_back(std::numeric_limits<double>::quiet_NaN());
  ys.push_back(1.0);

  std::string text = "# x,y\n";
  for (std::size_t i = 0; i < xs.size(); ++i) text += exact_text(xs[i]) + "," + exact_text(ys[i]) + "\n";

  const SaveResult saved = save_text(target, text);
  RC_REQUIRE(saved.ok);

  std::ifstream in(target, std::ios::binary);
  const rc::core::LogReport report = rc::core::read_log(in, ',');

  // Fifty good rows and the one with nan in it, which is counted and not kept.
  RC_REQUIRE_EQ(report.rows.size(), 50u);
  RC_CHECK_EQ(report.unparsable, 1);

  int identical = 0;
  for (std::size_t i = 0; i < report.rows.size(); ++i)
    if (report.rows[i][0] == xs[i] && report.rows[i][1] == ys[i]) ++identical;
  std::cout << "  50 poses written and read back through the 01-12 reader: " << identical
            << " identical, 1 nan counted as unparsable\n\n";
  RC_CHECK_EQ(identical, 50);
}
