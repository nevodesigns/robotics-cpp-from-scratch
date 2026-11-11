#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cstddef>
#include <cstdlib>
#include <string>
#include <vector>

// One thing that happens at a time in a scenario.
struct ScenarioEvent {
  enum class Kind { target, disturb, fault, end };
  double time = 0.0;
  Kind kind = Kind::target;
  double value = 0.0;       // the target, or the size of the push
  std::string text;         // for a fault: what to do, such as "sensor stuck"
  int line = 0;             // where in the file it came from
};

struct ScenarioError {
  int line = 0;
  std::string message;
};

struct Scenario {
  std::vector<ScenarioEvent> events;
  double duration() const { return events.empty() ? 0.0 : events.back().time; }
};

struct ParseResult {
  bool ok = false;
  std::vector<ScenarioError> errors;
};

// Split a line into words. Anything from a # to the end of the line is a comment.
inline std::vector<std::string> words_of(const std::string& line) {
  std::vector<std::string> out;
  std::string current;
  for (char c : line) {
    if (c == '#') break;
    if (c == ' ' || c == '\t' || c == '\r') {
      if (!current.empty()) out.push_back(current);
      current.clear();
    } else {
      current += c;
    }
  }
  if (!current.empty()) out.push_back(current);
  return out;
}

// A whole number or a decimal, and nothing else. False for an empty word, trailing characters, or a value that is not finite.
inline bool parse_number(const std::string& word, double& out) {
  if (word.empty()) return false;
  char* end = nullptr;
  const double v = std::strtod(word.c_str(), &end);
  if (end != word.c_str() + word.size()) return false;
  if (!(v == v) || v > 1e300 || v < -1e300) return false;
  out = v;
  return true;
}

// Read a scenario from text. One event to a line:
//   at <time> target <number>        set the target
//   at <time> disturb <number>       push the plant
//   at <time> fault <words...>       inject a fault
//   at <time> end                    the scenario is over (exactly one, and it is last)
// Blank lines and comments are ignored. Times may not be negative and may not go backwards. Every bad line is reported with its number, and the parse
// carries on to find the rest. If there is any error the scenario is left empty.
inline ParseResult parse_scenario(const std::string& text, Scenario& out) {
  // TODO: Clear out.events. Go through the text line by line (split at \\n; the last line may have no newline), counting lines from 1. words_of() splits a line and drops comments; skip a line with no words.
  // Every line is "at <time> <verb> ...". For each problem add a ScenarioError with the line number and a message (any words) and go on to the next line:
  // - it does not start with "at", or has fewer than 3 words;
  // - the time is not a number (parse_number), is negative, or is earlier than the last event's time;
  // - an event comes after the end;
  // - target and disturb need exactly one number after the verb; fault needs at least one word after it (join them with single spaces into `text`); end needs nothing after it (count it);
  // - any other verb is unknown.
  // A good line adds its event (with its line number) to a scenario of its own. After the last line: no end event is an error too (report it at the last line number).
  // If there were any errors return with ok false and leave `out` empty; otherwise copy the scenario into `out` and return ok.
  (void)text;
  (void)out;
  return ParseResult();
}

// The events that fall in the step from time t0 up to and including t1, but not at t0 itself: in file order, and so that stepping through
// consecutive intervals delivers every event once, and none twice.
inline std::vector<ScenarioEvent> events_between(const Scenario& s, double t0, double t1) {
  // TODO: the events with t0 < time <= t1, in the order they are in the scenario. Not t0 itself, and t1 itself
  (void)s;
  (void)t0;
  (void)t1;
  return std::vector<ScenarioEvent>();
}

#endif  // LESSON_SOLUTION_HPP
