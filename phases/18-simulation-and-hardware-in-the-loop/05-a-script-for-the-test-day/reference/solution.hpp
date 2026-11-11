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
  out.events.clear();
  ParseResult result;
  Scenario parsed;
  int line_number = 0;
  int ends = 0;
  double last_time = 0.0;
  std::size_t start = 0;
  while (start <= text.size()) {
    std::size_t stop = text.find('\n', start);
    if (stop == std::string::npos) stop = text.size();
    const std::string line = text.substr(start, stop - start);
    ++line_number;
    start = stop + 1;
    if (stop == text.size() && line.empty()) break;

    const std::vector<std::string> w = words_of(line);
    if (w.empty()) continue;
    const auto fail = [&](const std::string& why) { result.errors.push_back(ScenarioError{line_number, why}); };

    if (w[0] != "at" || w.size() < 3) {
      fail("expected: at <time> <what> ...");
      continue;
    }
    ScenarioEvent e;
    e.line = line_number;
    if (!parse_number(w[1], e.time)) {
      fail("the time is not a number: " + w[1]);
      continue;
    }
    if (e.time < 0.0) {
      fail("the time is negative");
      continue;
    }
    if (e.time < last_time) {
      fail("the time goes backwards");
      continue;
    }
    if (ends > 0) {
      fail("an event after the end");
      continue;
    }
    const std::string& verb = w[2];
    if (verb == "target" || verb == "disturb") {
      if (w.size() != 4) {
        fail(verb + " needs one number");
        continue;
      }
      if (!parse_number(w[3], e.value)) {
        fail("not a number: " + w[3]);
        continue;
      }
      e.kind = verb == "target" ? ScenarioEvent::Kind::target : ScenarioEvent::Kind::disturb;
    } else if (verb == "fault") {
      if (w.size() < 4) {
        fail("fault needs words saying which");
        continue;
      }
      e.kind = ScenarioEvent::Kind::fault;
      for (std::size_t i = 3; i < w.size(); ++i) e.text += (e.text.empty() ? "" : " ") + w[i];
    } else if (verb == "end") {
      if (w.size() != 3) {
        fail("end takes nothing after it");
        continue;
      }
      e.kind = ScenarioEvent::Kind::end;
      ++ends;
    } else {
      fail("unknown event: " + verb);
      continue;
    }
    last_time = e.time;
    parsed.events.push_back(e);
  }
  if (ends == 0 && result.errors.empty()) result.errors.push_back(ScenarioError{line_number, "the scenario has no end"});
  if (!result.errors.empty()) return result;
  out = parsed;
  result.ok = true;
  return result;
}

// The events that fall in the step from time t0 up to and including t1, but not at t0 itself: in file order, and so that stepping through
// consecutive intervals delivers every event once, and none twice.
inline std::vector<ScenarioEvent> events_between(const Scenario& s, double t0, double t1) {
  std::vector<ScenarioEvent> out;
  for (const ScenarioEvent& e : s.events)
    if (e.time > t0 && e.time <= t1) out.push_back(e);
  return out;
}

#endif  // LESSON_SOLUTION_HPP
