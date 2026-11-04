#include <rc/test/rc_test.hpp>

#include <cstdio>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

#include "solution.hpp"

namespace {

// A log with everything wrong with it that a real one has. Written with
// Windows line endings on purpose, because the machine on the bench had them.
const char* kMessyLog =
    "# encoder log, bench 3, 2026-11-04\r\n"
    "\r\n"
    "0.0,1.50,2.25\r\n"
    "0.1,1.55,2.30\r\n"
    "\r\n"
    "0.2,1.60\r\n"                 // a row cut short when the logger was killed
    "0.3,nan,2.40\r\n"             // a sensor that dropped out
    "0.4,1,70,2.45\r\n"            // written where the decimal is a comma
    "0.5,1.75,2.50\r\n";

std::string shown(const char* text) {
  std::string out;
  for (const char* c = text; *c != '\0'; ++c) out += (*c == '\r') ? ' ' : *c;
  return out;
}

}  // namespace

RC_TEST("std::stod answers a different question from the one you asked") {
  // What the obvious tool actually does, measured. Every row here returns a
  // number and does not complain.
  struct Case {
    const char* field;
    double stod_gives;
    std::size_t used;
  };

  std::cout << "\n  what std::stod does with a field\n\n";
  std::cout << "  " << std::left << std::setw(12) << "field" << std::setw(14)
            << "stod gives" << std::setw(14) << "chars used" << "parse_number\n"
            << std::right;

  const char* fields[] = {"1.5", "1,5", "1.5abc", "1.5.7", "nan", "inf", "1e3"};
  int stod_accepted = 0;
  int whole_field = 0;
  for (const char* field : fields) {
    const std::string text(field);
    std::size_t used = 0;
    double value = 0.0;
    bool threw = false;
    try {
      value = std::stod(text, &used);
    } catch (const std::exception&) {
      threw = true;
    }
    if (!threw) ++stod_accepted;

    const ParsedNumber parsed = parse_number(field);
    if (parsed.ok) ++whole_field;

    std::cout << "  " << std::left << std::setw(12) << field << std::setw(14);
    if (threw) std::cout << "threw";
    else std::cout << value;
    std::cout << std::setw(14);
    if (threw) std::cout << "-";
    else std::cout << (std::to_string(used) + " of " + std::to_string(text.size()));
    std::cout << (parsed.ok ? "accepts" : "REFUSES") << "\n" << std::right;
  }

  std::cout << "\n  stod accepted " << stod_accepted << " of 7, parse_number accepted "
            << whole_field << "\n\n";

  // stod takes every one of these: it throws only when the field does not
  // even begin with a number. parse_number takes the two that are wholly a
  // number and nothing else.
  RC_CHECK(stod_accepted == 7);
  RC_CHECK(whole_field == 2);
  RC_CHECK(parse_number("1.5").ok);
  RC_CHECK(parse_number("1e3").ok);

  // And the ones that matter are the quiet ones. A decimal comma reads as a
  // whole number, and nothing anywhere says so.
  RC_CHECK_NEAR(std::stod("1,5"), 1.0, 1e-12);
  RC_CHECK(!parse_number("1,5").ok);

  RC_CHECK(!parse_number("1.5abc").ok);
  RC_CHECK(!parse_number("1.5.7").ok);
  RC_CHECK(!parse_number("nan").ok);
  RC_CHECK(!parse_number("inf").ok);
}

RC_TEST("a field is a number only if the whole of it is one") {
  const ParsedNumber plain = parse_number("1.5");
  RC_CHECK(plain.ok);
  RC_CHECK_NEAR(plain.value, 1.5, 1e-12);

  // Whitespace around it is not part of the number, including the carriage
  // return a Windows written file leaves on the end of every line.
  RC_CHECK(parse_number("  1.5  ").ok);
  RC_CHECK(parse_number("\t2.5\t").ok);
  RC_CHECK_NEAR(parse_number("1.5\r").value, 1.5, 1e-12);

  // The ordinary spellings a logger produces.
  RC_CHECK_NEAR(parse_number("+1.5").value, 1.5, 1e-12);
  RC_CHECK_NEAR(parse_number("-2.25").value, -2.25, 1e-12);
  RC_CHECK_NEAR(parse_number("1e3").value, 1000.0, 1e-12);

  // Nothing at all is not a number, which is the empty field at the end of a
  // line with a trailing separator.
  RC_CHECK(!parse_number("").ok);
  RC_CHECK(!parse_number("   ").ok);

  // Out of range in either direction. strtod reports this through errno and
  // almost nobody looks.
  RC_CHECK(!parse_number("1e400").ok);
  RC_CHECK(!parse_number("1e-400").ok);

  // And one that is accepted and surprises people: strtod reads hexadecimal,
  // so a field of 0x10 is the number 16. That is a correct answer to a
  // question you probably did not mean to ask, and it is worth knowing before
  // you meet it rather than after.
  const ParsedNumber hex = parse_number("0x10");
  std::cout << "  0x10 parses, and it is " << hex.value << "\n\n";
  RC_CHECK(hex.ok);
  RC_CHECK_NEAR(hex.value, 16.0, 1e-12);
}

RC_TEST("a row says which field it could not use") {
  const ParsedRow good = parse_row("0.0,1.50,2.25", ',');
  RC_CHECK(good.ok);
  RC_CHECK(good.bad_field == -1);
  RC_CHECK(good.values.size() == 3u);
  RC_CHECK_NEAR(good.values[1], 1.50, 1e-12);

  // The position is counted from zero and reported, because "a line was bad"
  // is not something anybody can act on.
  //
  // And here is the one that a row on its own cannot catch. A reading written
  // where the decimal separator is a comma, in a comma separated file, does
  // not fail: it becomes an extra column, and every field in it is a perfectly
  // good number.
  const ParsedRow comma_decimal = parse_row("0.4,1,70,2.45", ',');
  RC_CHECK(comma_decimal.ok);
  RC_CHECK(comma_decimal.bad_field == -1);
  RC_CHECK(comma_decimal.values.size() == 4u);
  std::cout << "  0.4,1,70,2.45 parsed cleanly, as " << comma_decimal.values.size()
            << " fields rather than 3\n";

  // So does a row that was cut short when the logger was killed.
  const ParsedRow truncated = parse_row("0.2,1.60", ',');
  RC_CHECK(truncated.ok);
  RC_CHECK(truncated.values.size() == 2u);
  std::cout << "  0.2,1.60 parsed cleanly too, as " << truncated.values.size()
            << " fields. Only the column count catches either.\n\n";

  const ParsedRow dropped = parse_row("0.3,nan,2.40", ',');
  RC_CHECK(!dropped.ok);
  RC_CHECK(dropped.bad_field == 1);

  const ParsedRow trailing = parse_row("0.1,2.0,", ',');
  RC_CHECK(!trailing.ok);
  RC_CHECK(trailing.bad_field == 2);

  // An empty line has no fields and is not a row of nothing.
  RC_CHECK(!parse_row("", ',').ok);

  std::cout << "  a row with nan in the middle reports field "
            << dropped.bad_field << "\n";
  std::cout << "  a row with a trailing separator reports field "
            << trailing.bad_field << "\n\n";
}

RC_TEST("a whole log, counted rather than silently filtered") {
  std::istringstream input(kMessyLog);
  const LogReport report = read_log(input, ',');

  std::cout << "  the log as written\n\n";
  std::cout << shown(kMessyLog) << "\n";
  std::cout << "  " << std::left << std::setw(20) << "lines read" << report.lines_read
            << "\n";
  std::cout << "  " << std::left << std::setw(20) << "blank or comment" << report.skipped
            << "\n";
  std::cout << "  " << std::left << std::setw(20) << "field not a number"
            << report.unparsable << "\n";
  std::cout << "  " << std::left << std::setw(20) << "wrong width"
            << report.wrong_width << "\n";
  std::cout << "  " << std::left << std::setw(20) << "width taken as"
            << report.width << "\n";
  std::cout << "  " << std::left << std::setw(20) << "kept" << report.rows.size()
            << "\n";
  std::cout << "  " << std::left << std::setw(20) << "first bad line"
            << report.first_bad_line << "\n\n" << std::right;

  RC_CHECK(report.lines_read == 9);
  RC_CHECK(report.skipped == 3);          // the comment and two blanks
  RC_CHECK(report.width == 3u);           // from the first usable row

  // One field that was not a number: the nan.
  RC_CHECK(report.unparsable == 1);

  // Two rows that parsed perfectly and were the wrong shape: the truncated
  // one and the decimal comma one. Neither would have been caught by looking
  // at the fields.
  RC_CHECK(report.wrong_width == 2);
  RC_CHECK(report.rows.size() == 3u);

  // The line number is one based and counts every line in the file, including
  // the ones that were skipped, because that is what an editor shows.
  RC_CHECK(report.first_bad_line == 6);

  // The rows that survived are the right ones, in order.
  RC_REQUIRE(report.rows.size() == 3u);
  RC_CHECK_NEAR(report.rows[0][0], 0.0, 1e-12);
  RC_CHECK_NEAR(report.rows[1][1], 1.55, 1e-12);
  RC_CHECK_NEAR(report.rows[2][2], 2.50, 1e-12);

  // Worth dwelling on: only one of the three bad lines was caught by looking
  // at a field. The other two parsed without complaint and were caught by
  // counting columns, which is a check the row parser cannot make because it
  // does not know how wide the file is.
  std::cout << "  1 of the 3 bad lines had a field that was not a number\n";
  std::cout << "  the other 2 parsed cleanly and were the wrong shape\n\n";
}

RC_TEST("a log with nothing wrong with it reports nothing wrong") {
  std::istringstream clean("1.0,2.0\n3.0,4.0\n5.0,6.0\n");
  const LogReport report = read_log(clean, ',');

  RC_CHECK(report.lines_read == 3);
  RC_CHECK(report.skipped == 0);
  RC_CHECK(report.unparsable == 0);
  RC_CHECK(report.wrong_width == 0);
  RC_CHECK(report.first_bad_line == 0);
  RC_CHECK(report.rows.size() == 3u);
  RC_CHECK(report.width == 2u);

  // An empty file is not an error, it is an empty file.
  std::istringstream nothing("");
  const LogReport empty = read_log(nothing, ',');
  RC_CHECK(empty.lines_read == 0);
  RC_CHECK(empty.unparsable == 0);
  RC_CHECK(empty.wrong_width == 0);
  RC_CHECK(empty.width == 0u);
  RC_CHECK(empty.rows.empty());
  RC_CHECK(empty.first_bad_line == 0);
}

RC_TEST("the same reader, on a real file on disk") {
  // Everything above used a string, which is how the parsing is tested
  // quickly. This one proves the other end works, because a reader that takes
  // a stream can be handed either.
  const char* path = "rc_01_12_sample_log.csv";

  {
    std::ofstream out(path);
    RC_REQUIRE(out.good());
    out << "# written by this test\n";
    out << "0.0,1.5\n";
    out << "0.1,1.6\n";
    out << "0.2,oops\n";
  }

  std::ifstream in(path);
  RC_REQUIRE(in.good());
  const LogReport report = read_log(in, ',');
  in.close();
  std::remove(path);

  std::cout << "  read a file of 4 lines: kept " << report.rows.size() << ", skipped "
            << report.skipped << ", unparsable " << report.unparsable << "\n\n";

  RC_CHECK(report.lines_read == 4);
  RC_CHECK(report.skipped == 1);
  RC_CHECK(report.unparsable == 1);
  RC_CHECK(report.first_bad_line == 4);
  RC_REQUIRE(report.rows.size() == 2u);
  RC_CHECK_NEAR(report.rows[1][1], 1.6, 1e-12);
}
