#include <rc/test/rc_test.hpp>

#include <set>
#include <string>
#include <vector>

#include "solution.hpp"

RC_TEST("a well formed line parses") {
  const auto parsed = parse_setting("max_speed = 1.5");
  RC_REQUIRE(parsed.has_value());
  RC_CHECK_EQ(parsed.value().key, std::string("max_speed"));
  RC_CHECK_NEAR(parsed.value().value, 1.5, 1e-12);
}

RC_TEST("whitespace around the key and value is ignored") {
  const auto parsed = parse_setting("   wheel_base   =    0.30   ");
  RC_REQUIRE(parsed.has_value());
  RC_CHECK_EQ(parsed.value().key, std::string("wheel_base"));
  RC_CHECK_NEAR(parsed.value().value, 0.30, 1e-12);
}

RC_TEST("a line with no whitespace at all parses") {
  const auto parsed = parse_setting("loop_hz=100");
  RC_REQUIRE(parsed.has_value());
  RC_CHECK_NEAR(parsed.value().value, 100.0, 1e-12);
}

RC_TEST("a blank line reports Empty") {
  RC_REQUIRE(!parse_setting("").has_value());
  RC_CHECK(parse_setting("").error() == ConfigError::Empty);
  RC_CHECK(parse_setting("    \t  ").error() == ConfigError::Empty);
}

RC_TEST("a line without an equals sign reports MissingEquals") {
  const auto parsed = parse_setting("max_speed 1.5");
  RC_REQUIRE(!parsed.has_value());
  RC_CHECK(parsed.error() == ConfigError::MissingEquals);
}

RC_TEST("an unrecognised key reports UnknownKey") {
  const auto parsed = parse_setting("turbo = 11");
  RC_REQUIRE(!parsed.has_value());
  RC_CHECK(parsed.error() == ConfigError::UnknownKey);
}

RC_TEST("a value that is not a number reports NotANumber") {
  const auto parsed = parse_setting("max_speed = fast");
  RC_REQUIRE(!parsed.has_value());
  RC_CHECK(parsed.error() == ConfigError::NotANumber);
}

RC_TEST("a number with trailing rubbish is rejected, not silently truncated") {
  // The check that catches a stream state test without an eof test. Without it
  // this parses as 1.5 and the kg disappears without a word.
  const auto parsed = parse_setting("max_speed = 1.5kg");
  RC_REQUIRE(!parsed.has_value());
  RC_CHECK(parsed.error() == ConfigError::NotANumber);
}

RC_TEST("an empty value is not a number") {
  const auto parsed = parse_setting("max_speed =");
  RC_REQUIRE(!parsed.has_value());
  RC_CHECK(parsed.error() == ConfigError::NotANumber);
}

RC_TEST("zero and negative values report OutOfRange") {
  RC_REQUIRE(!parse_setting("max_speed = 0").has_value());
  RC_CHECK(parse_setting("max_speed = 0").error() == ConfigError::OutOfRange);
  RC_CHECK(parse_setting("loop_hz = -5").error() == ConfigError::OutOfRange);
}

RC_TEST("the checks happen in the documented order") {
  // An unknown key with a bad value must report the key, because that is the
  // first thing wrong and the first thing the operator should fix.
  const auto parsed = parse_setting("turbo = fast");
  RC_REQUIRE(!parsed.has_value());
  RC_CHECK(parsed.error() == ConfigError::UnknownKey);
}

RC_TEST("finding a setting answers the value") {
  const std::vector<Setting> settings = {{"max_speed", 1.5}, {"loop_hz", 100.0}};
  const std::optional<double> found = find_setting(settings, "loop_hz");
  RC_REQUIRE(found.has_value());
  RC_CHECK_NEAR(*found, 100.0, 1e-12);
}

RC_TEST("a missing setting answers nothing, and value_or supplies a default") {
  const std::vector<Setting> settings = {{"max_speed", 1.5}};
  RC_CHECK(!find_setting(settings, "loop_hz").has_value());
  RC_CHECK_NEAR(find_setting(settings, "loop_hz").value_or(50.0), 50.0, 1e-12);
  RC_CHECK_NEAR(find_setting(settings, "max_speed").value_or(50.0), 1.5, 1e-12);
}

RC_TEST("every error has a distinct message an operator can act on") {
  const ConfigError all[] = {ConfigError::Empty, ConfigError::MissingEquals,
                             ConfigError::UnknownKey, ConfigError::NotANumber,
                             ConfigError::OutOfRange};
  std::set<std::string> messages;
  for (const ConfigError error : all) {
    const std::string message = describe(error);
    RC_CHECK(!message.empty());
    messages.insert(message);
  }
  RC_CHECK_EQ(messages.size(), std::size_t{5});
}

RC_TEST("parsing a whole file keeps the good lines and explains the bad ones") {
  const std::vector<std::string> lines = {
      "max_speed = 1.2", "", "# not a comment format we support", "wheel_base = 0.31",
      "loop_hz = fast",  "turbo = 3",
  };

  std::vector<Setting> settings;
  std::vector<ConfigError> problems;
  for (const std::string& line : lines) {
    const auto parsed = parse_setting(line);
    if (parsed.has_value()) settings.push_back(parsed.value());
    else problems.push_back(parsed.error());
  }

  RC_CHECK_EQ(settings.size(), std::size_t{2});
  RC_CHECK_EQ(problems.size(), std::size_t{4});
  RC_CHECK_NEAR(find_setting(settings, "max_speed").value_or(0.0), 1.2, 1e-12);

  // A robot with a broken configuration line should still start with the
  // settings it did understand, and say what it could not.
  RC_CHECK_NEAR(find_setting(settings, "loop_hz").value_or(100.0), 100.0, 1e-12);
}
