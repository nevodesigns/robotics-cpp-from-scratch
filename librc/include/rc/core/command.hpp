// rc/core/command.hpp
//
// A command that is exactly one of several kinds, from lesson 03-13, graduated.
//
// std::variant holds exactly one of its alternatives, so a Stop cannot carry leftover wheel speeds and a Drive cannot be missing
// them, and std::visit calls the function for the kind it holds. A visitor with no function for one kind does not compile: a fourth kind
// added to the variant and not to Applier was refused by the compiler (the checks/ snippet of the lesson). A default-constructed variant
// holds its first alternative, the wrong question get_if gives nullptr and get throws std::bad_variant_access, and on the machine
// measured sizeof(Command) was 24 with the largest kind, Drive, at 16.
//
// A limit applies to what the wheels are doing now: driving at 0.8 and -0.4, then a limit of 0.5, left the wheels at 0.5 and -0.4.

#ifndef RC_CORE_COMMAND_HPP
#define RC_CORE_COMMAND_HPP

#include <sstream>
#include <string>
#include <variant>

namespace rc {
namespace core {

// Three kinds of command, and each carries exactly what it needs and nothing else.
struct Stop {};

struct Drive {
  double left = 0.0;
  double right = 0.0;
};

struct SetLimit {
  double max_speed = 1.0;
};

// A Command is exactly one of the three. It cannot be a Stop with wheel speeds left over in it, or a
// Drive with no speeds, and it cannot be none of them.
using Command = std::variant<Stop, Drive, SetLimit>;

struct RobotState {
  double left = 0.0;
  double right = 0.0;
  double limit = 1.0;
};

inline double clamp_speed(double v, double limit) {
  if (v > limit) return limit;
  if (v < -limit) return -limit;
  return v;
}

// One function for each kind. std::visit calls the one that matches what the command holds, and the
// compiler refuses to build the call if any kind has no function.
struct Describer {
  std::string operator()(const Stop&) const { return "stop"; }

  std::string operator()(const Drive& d) const {
    std::ostringstream out;
    out << "drive " << d.left << " " << d.right;
    return out.str();
  }

  std::string operator()(const SetLimit& s) const {
    std::ostringstream out;
    out << "limit " << s.max_speed;
    return out.str();
  }
};

inline std::string describe(const Command& command) { return std::visit(Describer{}, command); }

struct Applier {
  RobotState& state;

  void operator()(const Stop&) const {
    state.left = 0.0;
    state.right = 0.0;
  }

  void operator()(const Drive& d) const {
    state.left = clamp_speed(d.left, state.limit);
    state.right = clamp_speed(d.right, state.limit);
  }

  void operator()(const SetLimit& s) const {
    state.limit = s.max_speed;
    // A lower limit applies to what the wheels are doing now, not only to the next drive command.
    state.left = clamp_speed(state.left, state.limit);
    state.right = clamp_speed(state.right, state.limit);
  }
};

inline void apply(RobotState& state, const Command& command) { std::visit(Applier{state}, command); }

// Read one line: "stop", "drive <left> <right>" or "limit <speed above zero>". False for anything
// else, including a known word with the wrong number of values and anything left over at the end.
inline bool parse_command(const char* text, Command* out) {
  std::istringstream in(text);
  std::string word;
  if (!(in >> word)) return false;

  if (word == "stop") {
    std::string extra;
    if (in >> extra) return false;
    *out = Stop{};
    return true;
  }

  if (word == "drive") {
    Drive d;
    if (!(in >> d.left >> d.right)) return false;
    std::string extra;
    if (in >> extra) return false;
    *out = d;
    return true;
  }

  if (word == "limit") {
    SetLimit s;
    if (!(in >> s.max_speed) || s.max_speed <= 0.0) return false;
    std::string extra;
    if (in >> extra) return false;
    *out = s;
    return true;
  }

  return false;
}

}  // namespace core
}  // namespace rc

#endif  // RC_CORE_COMMAND_HPP
