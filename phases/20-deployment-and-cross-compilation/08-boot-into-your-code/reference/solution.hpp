#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <map>
#include <set>
#include <string>
#include <vector>

enum class Restart { no, on_failure, always };

// One program the robot runs at start-up, as a service of the operating system (the form is systemd's, which most robots run).
struct Service {
  std::string name;
  std::string exec;                       // the command, with its full path
  std::vector<std::string> after;         // services that must have started before this one
  std::vector<std::string> requires_;     // services this one cannot run without
  Restart restart = Restart::on_failure;
  int restart_sec = 1;                    // the wait before a restart
  int start_limit_burst = 5;              // starts allowed within the interval, after which the service is given up on
  int start_limit_interval_sec = 10;
  int watchdog_sec = 0;                   // the service must report within this long or be restarted; 0 for none
  long memory_max_mb = 0;                 // the memory it may use; 0 for no limit
};

struct UnitProblem {
  std::string service;
  std::string message;
};

// The text of a unit file for a service, one setting to a line, every line ending in a newline, in this order:
//   [Unit]
//   Description=<name>
//   After=<the services, joined by a space>                 (only if there are any)
//   Requires=<the services, joined by a space>              (only if there are any)
//   StartLimitIntervalSec=<seconds>
//   StartLimitBurst=<n>
//   (a blank line)
//   [Service]
//   ExecStart=<exec>
//   Restart=<no, on-failure or always>
//   RestartSec=<seconds>
//   WatchdogSec=<seconds>                                   (only if above 0)
//   MemoryMax=<n>M                                          (only if above 0)
//   (a blank line)
//   [Install]
//   WantedBy=multi-user.target
inline std::string render_unit(const Service& s) {
  auto join = [](const std::vector<std::string>& v) {
    std::string out;
    for (const std::string& x : v) out += (out.empty() ? "" : " ") + x;
    return out;
  };
  std::string t = "[Unit]\nDescription=" + s.name + "\n";
  if (!s.after.empty()) t += "After=" + join(s.after) + "\n";
  if (!s.requires_.empty()) t += "Requires=" + join(s.requires_) + "\n";
  t += "StartLimitIntervalSec=" + std::to_string(s.start_limit_interval_sec) + "\n";
  t += "StartLimitBurst=" + std::to_string(s.start_limit_burst) + "\n\n[Service]\n";
  t += "ExecStart=" + s.exec + "\n";
  t += std::string("Restart=") + (s.restart == Restart::no ? "no" : (s.restart == Restart::always ? "always" : "on-failure")) + "\n";
  t += "RestartSec=" + std::to_string(s.restart_sec) + "\n";
  if (s.watchdog_sec > 0) t += "WatchdogSec=" + std::to_string(s.watchdog_sec) + "\n";
  if (s.memory_max_mb > 0) t += "MemoryMax=" + std::to_string(s.memory_max_mb) + "M\n";
  t += "\n[Install]\nWantedBy=multi-user.target\n";
  return t;
}

namespace detail {
// Start whatever can be started, in order: each service after everything it names in `after` (names that are not services are ignored; a service that names itself can never start), and where two could go next, the smaller name. `done` is the set that could be started.
inline std::vector<std::string> order_services(const std::vector<Service>& services, std::set<std::string>& done) {
  std::set<std::string> names;
  for (const Service& s : services) names.insert(s.name);
  std::map<std::string, std::set<std::string>> waiting_for;
  for (const Service& s : services)
    for (const std::string& a : s.after)
      if (names.count(a) != 0) waiting_for[s.name].insert(a);
  std::vector<std::string> order;
  bool progressed = true;
  while (progressed) {
    progressed = false;
    for (const std::string& n : names) {
      if (done.count(n) != 0) continue;
      bool ready = true;
      for (const std::string& w : waiting_for[n])
        if (done.count(w) == 0) ready = false;
      if (ready) {
        order.push_back(n);
        done.insert(n);
        progressed = true;
        break;
      }
    }
  }
  return order;
}
}  // namespace detail

// The order to start the services in: each after everything it names in `after`, and where two could go first, the one with the smaller name. Names in `after` that are not services are ignored. If the ordering has a cycle, nothing can be started and the answer is empty.
inline std::vector<std::string> start_order(const std::vector<Service>& services) {
  std::set<std::string> names;
  for (const Service& s : services) names.insert(s.name);
  std::set<std::string> done;
  std::vector<std::string> order = detail::order_services(services, done);
  if (done.size() != names.size()) return {};
  return order;
}

// What is wrong with a set of services, in this order, each a UnitProblem on the service named:
//  - for each service in turn: "duplicate service name" if an earlier one has the name; "exec is not an absolute path" if exec does not start with '/'; for each name in `after` or `requires_` that is not a service, "after unknown service: <n>" or "requires unknown service: <n>";
//    for each name in `requires_` that is a service but not in `after`, "requires <n> but is not ordered after it"; "restart storm: restart with no wait" if restart is always and restart_sec is below 1;
//  - then, if start_order is empty although there are services, one UnitProblem on the first name (alphabetically) that cannot be started: "ordering cycle".
inline std::vector<UnitProblem> validate_services(const std::vector<Service>& services) {
  std::vector<UnitProblem> p;
  std::set<std::string> names;
  for (const Service& s : services) names.insert(s.name);
  std::set<std::string> seen;
  for (const Service& s : services) {
    if (!seen.insert(s.name).second) p.push_back({s.name, "duplicate service name"});
    if (s.exec.empty() || s.exec[0] != '/') p.push_back({s.name, "exec is not an absolute path"});
    for (const std::string& a : s.after)
      if (names.count(a) == 0) p.push_back({s.name, "after unknown service: " + a});
    for (const std::string& r : s.requires_) {
      if (names.count(r) == 0) {
        p.push_back({s.name, "requires unknown service: " + r});
      } else if (std::find(s.after.begin(), s.after.end(), r) == s.after.end()) {
        p.push_back({s.name, "requires " + r + " but is not ordered after it"});
      }
    }
    if (s.restart == Restart::always && s.restart_sec < 1) p.push_back({s.name, "restart storm: restart with no wait"});
  }
  if (!services.empty() && start_order(services).empty()) {
    std::set<std::string> done;
    detail::order_services(services, done);
    for (const std::string& n : names)
      if (done.count(n) == 0) {
        p.push_back({n, "ordering cycle"});
        break;
      }
  }
  return p;
}

// A service that crashes `lifetime` seconds after every start, restarted by the system. After its k-th crash (k from 0) the wait before the next start is min(first_delay * factor^k, max_delay), and k goes back to 0 for a run that lasted at least `stable_after`.
// The system gives up on it when a start would be the (burst + 1)-th within `interval` seconds: that start does not happen. `burst` of 0 or below means no limit.
struct RestartPolicy {
  double first_delay = 1.0;
  double factor = 1.0;
  double max_delay = 30.0;
  int burst = 5;
  double interval = 10.0;
  double stable_after = 30.0;
};

struct RestartRun {
  std::vector<double> starts;       // the times of the starts, the first at 0
  bool gave_up = false;
  double gave_up_at = 0.0;
};

// The starts up to `horizon` seconds. The first start is at 0. After each start the service runs `lifetime` seconds and crashes; the next start is that wait later. A start at or after the horizon does not count.
inline RestartRun simulate_restarts(const RestartPolicy& p, double lifetime, double horizon) {
  RestartRun r;
  double t = 0.0;
  int k = 0;
  while (t < horizon) {
    if (p.burst > 0) {
      int in_window = 0;
      for (double s : r.starts)
        if (t - s < p.interval) ++in_window;
      if (in_window >= p.burst) {
        r.gave_up = true;
        r.gave_up_at = t;
        return r;
      }
    }
    r.starts.push_back(t);
    if (lifetime >= p.stable_after) k = 0;
    const double wait = std::min(p.first_delay * std::pow(p.factor, k), p.max_delay);
    t += lifetime + wait;
    ++k;
  }
  return r;
}

#endif  // LESSON_SOLUTION_HPP
