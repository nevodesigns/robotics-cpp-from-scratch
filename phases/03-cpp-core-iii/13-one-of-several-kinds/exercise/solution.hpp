#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <sstream>
#include <string>
#include <variant>

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
  std::string operator()(const Stop&) const {
    // TODO: "stop".
    return "";
  }

  std::string operator()(const Drive& d) const {
    // TODO: "drive <left> <right>", built with a std::ostringstream: out << "drive " << d.left << " " << d.right;
    (void)d;
    return "";
  }

  std::string operator()(const SetLimit& s) const {
    // TODO: "limit <max_speed>" the same way.
    (void)s;
    return "";
  }
};

inline std::string describe(const Command& command) { return std::visit(Describer{}, command); }

struct Applier {
  RobotState& state;

  void operator()(const Stop&) const {
    // TODO: both wheel speeds to zero.
  }

  void operator()(const Drive& d) const {
    // TODO: state.left and state.right become the command's speeds, each held to plus or minus
    // state.limit with clamp_speed.
    (void)d;
  }

  void operator()(const SetLimit& s) const {
    // TODO: state.limit becomes s.max_speed, and the speeds the wheels have NOW are clamped to it too:
    // a lower limit applies to what is happening, not only to the next drive command.
    (void)s;
  }
};

inline void apply(RobotState& state, const Command& command) { std::visit(Applier{state}, command); }

// Read one line: "stop", "drive <left> <right>" or "limit <speed above zero>". False for anything
// else, including a known word with the wrong number of values and anything left over at the end.
inline bool parse_command(const char* text, Command* out) {
  // TODO: put the text in a std::istringstream and read the first word with >>. "stop" takes no
  // values, "drive" takes two doubles, "limit" takes one double above zero. After the values, read
  // one more std::string: if that succeeds there was something left over, so return false.
  // On success assign *out = Stop{} (or the Drive or SetLimit you filled in) and return true.
  (void)text;
  (void)out;
  return false;
}

#endif  // LESSON_SOLUTION_HPP
