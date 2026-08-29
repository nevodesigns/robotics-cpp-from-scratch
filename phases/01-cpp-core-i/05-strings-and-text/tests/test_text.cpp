#include <rc/test/rc_test.hpp>

#include <string>
#include <vector>

#include "solution.hpp"

RC_TEST("trimming removes whitespace from both ends") {
  RC_CHECK_EQ(trim("  hello  "), std::string("hello"));
  RC_CHECK_EQ(trim("\t max_speed \r\n"), std::string("max_speed"));
}

RC_TEST("trimming leaves inner spaces alone") {
  RC_CHECK_EQ(trim("  max speed  "), std::string("max speed"));
}

RC_TEST("trimming text that is all whitespace gives nothing") {
  // The case that needs the npos check. Without it this reads past the end.
  RC_CHECK_EQ(trim("     "), std::string(""));
  RC_CHECK_EQ(trim("\t\r\n"), std::string(""));
  RC_CHECK_EQ(trim(""), std::string(""));
}

RC_TEST("trimming text with no whitespace changes nothing") {
  RC_CHECK_EQ(trim("imu.temp"), std::string("imu.temp"));
}

RC_TEST("a prefix is recognised") {
  RC_CHECK(starts_with("ttyUSB0", "tty"));
  RC_CHECK(starts_with("tty", "tty"));
  RC_CHECK(!starts_with("ttyUSB0", "usb"));
}

RC_TEST("a prefix longer than the text is not a prefix") {
  // The check that catches comparing before testing the length.
  RC_CHECK(!starts_with("tty", "ttyUSB0"));
  RC_CHECK(!starts_with("", "tty"));
}

RC_TEST("an empty prefix matches anything") {
  RC_CHECK(starts_with("ttyUSB0", ""));
  RC_CHECK(starts_with("", ""));
}

RC_TEST("a suffix is recognised") {
  RC_CHECK(ends_with("readings.log", ".log"));
  RC_CHECK(ends_with(".log", ".log"));
  RC_CHECK(!ends_with("readings.log", ".txt"));
  RC_CHECK(!ends_with(".log", "readings.log"));
}

RC_TEST("splitting cuts on the separator") {
  const std::vector<std::string> parts = split("imu.temp.raw", '.');
  RC_REQUIRE_EQ(parts.size(), std::size_t{3});
  RC_CHECK_EQ(parts[0], std::string("imu"));
  RC_CHECK_EQ(parts[1], std::string("temp"));
  RC_CHECK_EQ(parts[2], std::string("raw"));
}

RC_TEST("text with no separator is one piece") {
  const std::vector<std::string> parts = split("imu", '.');
  RC_REQUIRE_EQ(parts.size(), std::size_t{1});
  RC_CHECK_EQ(parts[0], std::string("imu"));
}

RC_TEST("two separators in a row produce an empty piece") {
  // A parser needs to see that a field was blank. Silently dropping it turns a
  // malformed line into a plausible one.
  const std::vector<std::string> parts = split("a,,b", ',');
  RC_REQUIRE_EQ(parts.size(), std::size_t{3});
  RC_CHECK_EQ(parts[1], std::string(""));
}

RC_TEST("a leading or trailing separator produces an empty piece too") {
  RC_CHECK_EQ(split(",a", ',').size(), std::size_t{2});
  RC_CHECK_EQ(split("a,", ',').size(), std::size_t{2});
  RC_CHECK_EQ(split(",", ',').size(), std::size_t{2});
}

RC_TEST("splitting nothing gives nothing") {
  RC_CHECK(split("", '.').empty());
}

RC_TEST("joining puts the pieces back") {
  RC_CHECK_EQ(join({"imu", "temp", "raw"}, '.'), std::string("imu.temp.raw"));
}

RC_TEST("joining puts no separator after the last piece") {
  RC_CHECK_EQ(join({"a"}, ','), std::string("a"));
  RC_CHECK_EQ(join({}, ','), std::string(""));
}

RC_TEST("splitting then joining returns the original") {
  const std::string original = "battery.volts.raw";
  RC_CHECK_EQ(join(split(original, '.'), '.'), original);

  const std::string awkward = ",a,,b,";
  RC_CHECK_EQ(join(split(awkward, ','), ','), awkward);
}

RC_TEST("characters are counted") {
  RC_CHECK_EQ(count_char("imu.temp.raw", '.'), 2);
  RC_CHECK_EQ(count_char("imu", '.'), 0);
  RC_CHECK_EQ(count_char("", '.'), 0);
}

RC_TEST("a view parameter accepts a literal and a string without copying") {
  // The reason every reading parameter is a view: all three of these call the
  // same function and none of them creates a temporary string.
  const std::string owned = "imu.temp";
  const std::string_view viewed = owned;
  RC_CHECK_EQ(count_char(owned, '.'), 1);
  RC_CHECK_EQ(count_char("imu.temp", '.'), 1);
  RC_CHECK_EQ(count_char(viewed, '.'), 1);
}
