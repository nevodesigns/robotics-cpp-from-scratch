#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <optional>
#include <sstream>
#include <string>
#include <vector>

#include <rc/core/compat.hpp>

// Why a configuration line could not be turned into a setting.
enum class ConfigError {
  Empty,
  MissingEquals,
  UnknownKey,
  NotANumber,
  OutOfRange,
};

struct Setting {
  std::string key;
  double value = 0.0;
};

// Removes whitespace from both ends. Provided for you.
inline std::string trimmed(const std::string& text) {
  const std::size_t first = text.find_first_not_of(" \t\r\n");
  if (first == std::string::npos) return "";
  const std::size_t last = text.find_last_not_of(" \t\r\n");
  return text.substr(first, last - first + 1);
}

// The keys this robot understands. Provided for you.
inline bool is_known_key(const std::string& key) {
  return key == "max_speed" || key == "wheel_base" || key == "loop_hz";
}

// Turns one line, such as "max_speed = 1.5", into a Setting, or explains why it
// could not. Check in the order the errors are listed in docs/en.md.
//
// Return a failure with:
//   return rc::unexpected(ConfigError::MissingEquals);
inline rc::expected<Setting, ConfigError> parse_setting(const std::string& line) {
  // TODO
  (void)line;
  return rc::unexpected(ConfigError::Empty);
}

// The value stored under this key, or nothing.
inline std::optional<double> find_setting(const std::vector<Setting>& settings,
                                          const std::string& key) {
  // TODO
  (void)settings;
  (void)key;
  return std::nullopt;
}

// A message an operator can act on. Every error must produce a distinct,
// non empty message.
inline std::string describe(ConfigError error) {
  // TODO
  (void)error;
  return "";
}

#endif  // LESSON_SOLUTION_HPP
