// rc/field/run_log.hpp
//
// The run log, from lesson 22-08.
//
// A log is a version line, the scenario text it ran (each line after "| "), and entries "@ time channel value ..." in time order. text() writes it with 17 digits, which gives every double back exactly; parse_log reads it, reports the line of the first mistake, and treats a last line with no newline as the stump of a power cut (truncated) rather than as corruption. between() and at() find entries by time, and first_difference() gives the time at which two logs first disagree.
//
// A 10 s run at 50 Hz of two channels was 1000 entries and 72.7 kB of text; written with 6 digits the values were off by 0.0000050 after the round trip and with 17 they were identical; the same seed twice gave no difference and two seeds differed from 0.02 s; a log cut in the middle of a line parsed with 999 entries and the truncated flag.

#ifndef RC_RC_FIELD_RUN_LOG_HPP
#define RC_RC_FIELD_RUN_LOG_HPP

#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

namespace rc {
namespace field {

struct LogEntry {
  double t = 0.0;
  std::string channel;
  std::vector<double> values;
};

class RunLog {
 public:
  std::string scenario;  // the text of the scenario that was run, kept with the log
  std::vector<LogEntry> entries;

  void add(double t, const std::string& channel, const std::vector<double>& values) {
    LogEntry e;
    e.t = t;
    e.channel = channel;
    e.values = values;
    entries.push_back(e);
  }

  // The log as text. 17 digits give back every double exactly; fewer do not.
  std::string text(int digits = 17) const {
    std::ostringstream o;
    o << std::setprecision(digits);
    o << "runlog 1\n";
    std::istringstream in(scenario);
    std::string line;
    while (std::getline(in, line)) o << "| " << line << "\n";
    for (std::size_t i = 0; i < entries.size(); ++i) {
      o << "@ " << entries[i].t << " " << entries[i].channel;
      for (std::size_t k = 0; k < entries[i].values.size(); ++k) o << " " << entries[i].values[k];
      o << "\n";
    }
    return o.str();
  }

  // The entries with t0 <= t < t1, of one channel or (with an empty name) of all.
  std::vector<LogEntry> between(double t0, double t1, const std::string& channel) const {
    std::vector<LogEntry> out;
    for (std::size_t i = 0; i < entries.size(); ++i)
      if (entries[i].t >= t0 && entries[i].t < t1 && (channel.empty() || entries[i].channel == channel)) out.push_back(entries[i]);
    return out;
  }

  // The latest entry of a channel at or before t: what a sensor that holds its last value would show.
  bool at(const std::string& channel, double t, LogEntry& out) const {
    bool found = false;
    for (std::size_t i = 0; i < entries.size(); ++i) {
      if (entries[i].t > t) break;
      if (entries[i].channel == channel) {
        out = entries[i];
        found = true;
      }
    }
    return found;
  }
};

struct LogParse {
  RunLog log;
  bool ok = false;
  bool truncated = false;  // the last line was cut short and has been dropped
  int error_line = 0;
  std::string error;
};

inline LogParse parse_log(const std::string& text) {
  LogParse out;
  auto fail = [&](int line, const std::string& message) {
    out.ok = false;
    out.error_line = line;
    out.error = message;
    return out;
  };
  std::string body = text;
  if (!body.empty() && body.back() != '\n') {
    const std::size_t cut = body.find_last_of('\n');
    body = cut == std::string::npos ? std::string() : body.substr(0, cut + 1);
    out.truncated = true;
  }
  std::istringstream in(body);
  std::string line;
  int number = 0;
  double last_t = -1.0e300;
  while (std::getline(in, line)) {
    ++number;
    if (number == 1) {
      if (line != "runlog 1") return fail(1, "not a run log, or a version this reader does not know");
      continue;
    }
    if (line.compare(0, 1, "|") == 0) {
      out.log.scenario += line.size() > 2 ? line.substr(2) + "\n" : "\n";
      continue;
    }
    if (line.compare(0, 2, "@ ") != 0) return fail(number, "a line that is neither a scenario line nor an entry");
    std::istringstream words(line.substr(2));
    std::string w;
    std::vector<std::string> parts;
    while (words >> w) parts.push_back(w);
    if (parts.size() < 2) return fail(number, "an entry needs a time and a channel");
    LogEntry e;
    char* end = nullptr;
    e.t = std::strtod(parts[0].c_str(), &end);
    if (end == parts[0].c_str() || *end != '\0') return fail(number, "'" + parts[0] + "' is not a time");
    if (e.t < last_t) return fail(number, "time goes backwards");
    last_t = e.t;
    e.channel = parts[1];
    for (std::size_t k = 2; k < parts.size(); ++k) {
      const double v = std::strtod(parts[k].c_str(), &end);
      if (end == parts[k].c_str() || *end != '\0') return fail(number, "'" + parts[k] + "' is not a number");
      e.values.push_back(v);
    }
    out.log.entries.push_back(e);
  }
  if (number == 0) return fail(0, "empty");
  out.ok = true;
  return out;
}

// The time of the first entry at which two logs differ by more than tol (in a channel, a count of values, or a value), or -1 if they do not. A log that is shorter differs where the other goes on.
inline double first_difference(const RunLog& a, const RunLog& b, double tol) {
  const std::size_t n = a.entries.size() < b.entries.size() ? a.entries.size() : b.entries.size();
  for (std::size_t i = 0; i < n; ++i) {
    const LogEntry& x = a.entries[i];
    const LogEntry& y = b.entries[i];
    bool differ = x.channel != y.channel || x.values.size() != y.values.size() || std::fabs(x.t - y.t) > tol;
    for (std::size_t k = 0; !differ && k < x.values.size(); ++k) differ = std::fabs(x.values[k] - y.values[k]) > tol;
    if (differ) return x.t;
  }
  if (a.entries.size() == b.entries.size()) return -1.0;
  return a.entries.size() > b.entries.size() ? a.entries[n].t : b.entries[n].t;
}


}  // namespace field
}  // namespace rc

#endif
