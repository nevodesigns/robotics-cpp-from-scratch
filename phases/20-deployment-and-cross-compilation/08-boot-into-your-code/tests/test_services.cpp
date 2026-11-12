#include <rc/test/rc_test.hpp>

#include <iomanip>
#include <iostream>

#include "solution.hpp"

namespace {

Service svc(const char* name, std::vector<std::string> after = {}, std::vector<std::string> requires_ = {}) {
  Service s;
  s.name = name;
  s.exec = std::string("/opt/robot/bin/") + name;
  s.after = std::move(after);
  s.requires_ = std::move(requires_);
  return s;
}

std::string joined(const std::vector<std::string>& v) {
  std::string s;
  for (const std::string& x : v) s += (s.empty() ? "" : " -> ") + x;
  return s.empty() ? "(none)" : s;
}

std::string problems(const std::vector<UnitProblem>& p) {
  std::string s;
  for (const UnitProblem& x : p) s += "\n    " + x.service + ": " + x.message;
  return p.empty() ? " none" : s;
}

}  // namespace

RC_TEST("a unit file is written from the spec, one setting to a line, and leaves out what it has nothing to say about") {
  Service s = svc("perception", {"drivers"}, {"drivers"});
  s.restart = Restart::on_failure;
  s.restart_sec = 3;
  s.watchdog_sec = 10;
  s.memory_max_mb = 512;
  const std::string text = render_unit(s);
  std::cout << text << "\n";
  const std::string want =
      "[Unit]\nDescription=perception\nAfter=drivers\nRequires=drivers\nStartLimitIntervalSec=10\nStartLimitBurst=5\n\n"
      "[Service]\nExecStart=/opt/robot/bin/perception\nRestart=on-failure\nRestartSec=3\nWatchdogSec=10\nMemoryMax=512M\n\n"
      "[Install]\nWantedBy=multi-user.target\n";
  RC_CHECK(text == want);
  const std::string bare = render_unit(svc("drivers"));
  RC_CHECK(bare.find("After=") == std::string::npos && bare.find("Requires=") == std::string::npos);
  RC_CHECK(bare.find("WatchdogSec") == std::string::npos && bare.find("MemoryMax") == std::string::npos);
  Service a = svc("x");
  a.restart = Restart::always;
  RC_CHECK(render_unit(a).find("Restart=always") != std::string::npos);
  a.restart = Restart::no;
  RC_CHECK(render_unit(a).find("Restart=no\n") != std::string::npos);
}

RC_TEST("services start in an order that respects what they wait for, the smaller name first when it does not matter, and a cycle starts nothing") {
  const std::vector<Service> robot = {svc("planner", {"perception"}), svc("perception", {"drivers"}), svc("drivers"), svc("logger"), svc("bridge", {"planner", "logger"})};
  const std::vector<std::string> order = start_order(robot);
  std::cout << "  start order: " << joined(order) << "\n";
  RC_CHECK(order.size() == 5);
  if (order.size() != 5) return;
  RC_CHECK(order[0] == "bridge" || order[0] == "drivers" || order[0] == "logger");
  RC_CHECK(joined(order) == "drivers -> logger -> perception -> planner -> bridge");
  const std::vector<Service> loop = {svc("a", {"b"}), svc("b", {"c"}), svc("c", {"a"}), svc("d")};
  RC_CHECK(start_order(loop).empty());
  RC_CHECK(start_order({svc("self", {"self"})}).empty());
  RC_CHECK(start_order({svc("only", {"nothing"})}).size() == 1);     // a name that is not a service is ignored here
  RC_CHECK(start_order({}).empty());
  std::cout << "  a cycle of three: nothing starts\n\n";
}

RC_TEST("a set of services that would not start, or would start in the wrong order, is named in full before it is installed") {
  std::vector<Service> v = {svc("drivers"), svc("perception", {"drivers"}, {"drivers"}), svc("planner", {}, {"perception"}), svc("logger", {"ghost"}, {"phantom"})};
  v[1].exec = "perception";
  v[3].restart = Restart::always;
  v[3].restart_sec = 0;
  v.push_back(svc("drivers"));
  const std::vector<UnitProblem> p = validate_services(v);
  std::cout << "  " << p.size() << " problems:" << problems(p) << "\n";
  RC_CHECK(p.size() == 6);
  if (p.size() != 6) return;
  RC_CHECK(p[0].service == "perception" && p[0].message == "exec is not an absolute path");
  RC_CHECK(p[1].service == "planner" && p[1].message == "requires perception but is not ordered after it");
  RC_CHECK(p[2].service == "logger" && p[2].message == "after unknown service: ghost");
  RC_CHECK(p[3].service == "logger" && p[3].message == "requires unknown service: phantom");
  RC_CHECK(p[4].service == "logger" && p[4].message == "restart storm: restart with no wait");
  RC_CHECK(p[5].service == "drivers" && p[5].message == "duplicate service name");
  const std::vector<UnitProblem> cyc = validate_services({svc("a", {"b"}), svc("b", {"a"})});
  RC_CHECK(cyc.size() == 1 && cyc[0].service == "a" && cyc[0].message == "ordering cycle");
  RC_CHECK(validate_services({svc("drivers"), svc("perception", {"drivers"}, {"drivers"})}).empty());
  RC_CHECK(validate_services({}).empty());
  std::cout << "\n";
}

RC_TEST("a service that crashes at once, restarted immediately, starts hundreds of times a minute, and a back-off starts it six times") {
  RestartPolicy immediate;
  immediate.first_delay = 0.1;
  immediate.factor = 1.0;
  immediate.burst = 0;                       // no start limit
  RestartPolicy backoff;
  backoff.first_delay = 1.0;
  backoff.factor = 2.0;
  backoff.max_delay = 30.0;
  backoff.burst = 5;
  backoff.interval = 10.0;
  RestartPolicy limited = immediate;
  limited.burst = 5;
  limited.interval = 10.0;
  const double lifetime = 0.2;
  const RestartRun a = simulate_restarts(immediate, lifetime, 60.0);
  const RestartRun b = simulate_restarts(backoff, lifetime, 60.0);
  const RestartRun c = simulate_restarts(limited, lifetime, 60.0);
  std::cout << "  a service that crashes 0.2 s after every start, watched for 60 s:\n";
  std::cout << "  restarted after 0.1 s, no limit:            " << a.starts.size() << " starts\n";
  std::cout << "  waits 1, 2, 4, 8, 16, 30 s, limit 5 in 10 s: " << b.starts.size() << " starts, " << (b.gave_up ? "gave up" : "still being tried") << "\n";
  std::cout << "  restarted after 0.1 s, limit 5 in 10 s:     " << c.starts.size() << " starts, " << (c.gave_up ? "gave up at " + std::to_string(c.gave_up_at).substr(0, 4) + " s" : "still being tried") << "\n\n";
  RC_CHECK(a.starts.size() >= 200 && a.starts.size() <= 201);       // 60 s at 0.3 s a start, and the sum of 0.3 added 200 times falls just under 60
  RC_CHECK(!a.gave_up);
  RC_CHECK(b.starts.size() == 6 && !b.gave_up);
  RC_CHECK_NEAR(b.starts[1], 1.2, 1e-9);
  RC_CHECK_NEAR(b.starts[2], 3.4, 1e-9);
  RC_CHECK_NEAR(b.starts[5], 32.0, 1e-9);
  RC_CHECK(c.starts.size() == 5 && c.gave_up);
  RC_CHECK_NEAR(c.gave_up_at, 1.5, 1e-9);
}

RC_TEST("a back-off that never resets punishes a service that has been healthy for a day, and one that resets after a stable run does not") {
  RestartPolicy keeps;
  keeps.first_delay = 1.0;
  keeps.factor = 2.0;
  keeps.max_delay = 30.0;
  keeps.burst = 0;
  keeps.stable_after = 1e9;                  // never resets
  RestartPolicy resets = keeps;
  resets.stable_after = 30.0;
  // Runs for 100 s, then crashes, over and over: the wait before the restart.
  const RestartRun a = simulate_restarts(keeps, 100.0, 1000.0);
  const RestartRun b = simulate_restarts(resets, 100.0, 1000.0);
  std::cout << "  a service that runs 100 s between crashes, watched for 1000 s: with a back-off that never resets " << a.starts.size() << " starts (the waits grow to 30 s); with one that resets after a run of 30 s " << b.starts.size() << " (a wait of 1 s each time)\n\n";
  RC_CHECK(b.starts.size() > a.starts.size());
  RC_CHECK(b.starts.size() == 10);
  RC_CHECK(a.starts.size() == 9);
  RC_CHECK_NEAR(b.starts[1] - b.starts[0], 101.0, 1e-9);
}
