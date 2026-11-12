#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

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
  (void)fail;
  (void)text;  // TODO: drop an unfinished last line (and set truncated), check the first line, collect the scenario lines, read each entry with its time and values, and stop with fail() at the first line that is wrong
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


#endif
