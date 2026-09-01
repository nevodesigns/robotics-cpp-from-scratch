// rc/core/config.hpp
//
// The configuration parser from lesson 03-02, graduated, and the error style
// used for the rest of the curriculum.
//
// A missing key is a question with the answer no, which is what std::optional
// says. A malformed file is a failure with a reason, which is what
// rc::expected says. Neither is exceptional and neither needs an exception.

#ifndef RC_CORE_CONFIG_HPP
#define RC_CORE_CONFIG_HPP

#include <optional>
#include <sstream>
#include <string>
#include <vector>
#include <rc/core/compat.hpp>

namespace rc {
namespace core {

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

inline std::string trimmed(const std::string& text) {
  const std::size_t first = text.find_first_not_of(" \t\r\n");
  if (first == std::string::npos) return "";
  const std::size_t last = text.find_last_not_of(" \t\r\n");
  return text.substr(first, last - first + 1);
}

inline bool is_known_key(const std::string& key) {
  return key == "max_speed" || key == "wheel_base" || key == "loop_hz";
}

inline rc::expected<Setting, ConfigError> parse_setting(const std::string& line) {
  const std::string content = trimmed(line);
  if (content.empty()) return rc::unexpected(ConfigError::Empty);

  const std::size_t equals = content.find('=');
  if (equals == std::string::npos) return rc::unexpected(ConfigError::MissingEquals);

  const std::string key = trimmed(content.substr(0, equals));
  if (!is_known_key(key)) return rc::unexpected(ConfigError::UnknownKey);

  const std::string value_text = trimmed(content.substr(equals + 1));

  // A stream rather than std::stod, because stod throws on bad input and the
  // whole point of this function is to return the failure instead.
  std::istringstream in(value_text);
  double value = 0.0;
  in >> value;

  // Both checks are needed. The first catches text that is not a number at all.
  // The second catches text that starts with one and continues, such as 1.5kg,
  // which would otherwise parse as 1.5 and silently discard the rest.
  if (in.fail() || !in.eof()) return rc::unexpected(ConfigError::NotANumber);

  if (!(value > 0.0)) return rc::unexpected(ConfigError::OutOfRange);

  Setting setting;
  setting.key = key;
  setting.value = value;
  return setting;
}

inline std::optional<double> find_setting(const std::vector<Setting>& settings,
                                          const std::string& key) {
  for (const Setting& setting : settings) {
    if (setting.key == key) return setting.value;
  }
  // Nothing found is not a failure worth explaining, so optional is the right
  // shape here and expected would be needless ceremony.
  return std::nullopt;
}

inline std::string describe(ConfigError error) {
  switch (error) {
    case ConfigError::Empty:
      return "the line is blank, so there is nothing to configure";
    case ConfigError::MissingEquals:
      return "expected a line of the form key = value";
    case ConfigError::UnknownKey:
      return "unknown setting, expected max_speed, wheel_base or loop_hz";
    case ConfigError::NotANumber:
      return "the value is not a number";
    case ConfigError::OutOfRange:
      return "the value must be greater than zero";
  }
  // Reached only if somebody adds an enumerator and forgets this function. The
  // switch has no default on purpose, so most compilers warn at that point
  // rather than leaving it to be discovered by an operator.
  return "unrecognised configuration error";
}

}  // namespace core
}  // namespace rc

#endif  // RC_CORE_CONFIG_HPP
