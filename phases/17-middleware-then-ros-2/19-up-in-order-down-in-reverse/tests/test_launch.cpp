#include <rc/test/rc_test.hpp>

#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

#include "solution.hpp"

namespace {

std::string joined(const std::vector<std::string>& v) {
  std::string s;
  for (const std::string& x : v) s += (s.empty() ? "" : " ") + x;
  return s;
}

// The robot's graph, listed in no helpful order: driver, filter, planner and controller each need the one before, and the logger and viewer are optional.
void fill(Launcher& l) {
  l.add("planner", {"filter"}, true);
  l.add("controller", {"planner"}, true);
  l.add("filter", {"driver"}, true);
  l.add("driver", {}, true);
  l.add("logger", {"driver"}, false);
  l.add("viewer", {"planner"}, false);
}

std::vector<NodeSpec> specs() {
  std::vector<NodeSpec> s;
  s.push_back(NodeSpec{"planner", {"filter"}, true});
  s.push_back(NodeSpec{"controller", {"planner"}, true});
  s.push_back(NodeSpec{"filter", {"driver"}, true});
  s.push_back(NodeSpec{"driver", {}, true});
  s.push_back(NodeSpec{"logger", {"driver"}, false});
  s.push_back(NodeSpec{"viewer", {"planner"}, false});
  return s;
}

}  // namespace

RC_TEST("every node starts after everything it needs, and of those that could go next the one listed first goes first") {
  std::string error;
  const std::vector<std::string> order = start_order(specs(), error);
  std::cout << "  listed planner, controller, filter, driver, logger, viewer; start order: " << joined(order) << "\n\n";
  RC_CHECK(error.empty());
  RC_CHECK(order.size() == 6);
  RC_CHECK(joined(order) == "driver filter planner controller logger viewer");
}

RC_TEST("a list with a repeated name, a missing need or a circle of needs is refused with the reason, and gives no order") {
  std::string error;
  std::vector<NodeSpec> dup = specs();
  dup.push_back(NodeSpec{"driver", {}, true});
  RC_CHECK(start_order(dup, error).empty());
  const std::string e1 = error;
  std::vector<NodeSpec> missing = specs();
  missing.push_back(NodeSpec{"extra", {"nowhere"}, true});
  RC_CHECK(start_order(missing, error).empty());
  const std::string e2 = error;
  std::vector<NodeSpec> circle;
  circle.push_back(NodeSpec{"a", {"b"}, true});
  circle.push_back(NodeSpec{"b", {"c"}, true});
  circle.push_back(NodeSpec{"c", {"a"}, true});
  RC_CHECK(start_order(circle, error).empty());
  const std::string e3 = error;
  std::cout << "  repeated name: " << e1 << "\n  missing need: " << e2 << "\n  circle: " << e3 << "\n\n";
  RC_CHECK(e1 == "duplicate node: driver");
  RC_CHECK(e2 == "extra needs nowhere, which is not listed");
  RC_CHECK(e3 == "the needs of the nodes go round in a circle");
  RC_CHECK(start_order(std::vector<NodeSpec>(), error).empty() && error.empty());     // nothing to start is not an error
}

RC_TEST("everything is configured before anything is activated, and a clean shut down is the reverse of the start") {
  Launcher l;
  fill(l);
  const LaunchReport up = l.bring_up();
  std::cout << "  bring up: " << joined(up.log) << "\n  running: " << joined(up.running) << "\n";
  RC_CHECK(up.ok);
  RC_CHECK(up.running.size() == 6);
  RC_CHECK(joined(up.log) == "driver:configure filter:configure planner:configure controller:configure logger:configure viewer:configure "
                             "driver:activate filter:activate planner:activate controller:activate logger:activate viewer:activate");
  RC_CHECK(l.state_of("controller") == LifeState::active);
  const LaunchReport down = l.shut_down();
  std::cout << "  shut down: " << joined(down.log) << "\n\n";
  RC_CHECK(joined(down.log) == "viewer:deactivate viewer:cleanup logger:deactivate logger:cleanup controller:deactivate controller:cleanup "
                               "planner:deactivate planner:cleanup filter:deactivate filter:cleanup driver:deactivate driver:cleanup");
  RC_CHECK(l.state_of("driver") == LifeState::unconfigured);
}

RC_TEST("when a required node cannot configure, nothing is activated and what was configured is given back, newest first") {
  Launcher g;
  g.add("planner", {"filter"}, true).on(Transition::configure, []() { return CallbackResult::failure; });
  g.add("controller", {"planner"}, true);
  g.add("filter", {"driver"}, true);
  g.add("driver", {}, true);
  const LaunchReport r = g.bring_up();
  std::cout << "  planner will not configure: ok " << (r.ok ? "yes" : "no") << ", failed '" << r.failed << "', log: " << joined(r.log) << "\n\n";
  RC_CHECK(!r.ok);
  RC_CHECK(r.failed == "planner");
  RC_CHECK(joined(r.log) == "driver:configure filter:configure planner:configure filter:cleanup driver:cleanup");
  RC_CHECK(g.state_of("driver") == LifeState::unconfigured);
  RC_CHECK(g.state_of("filter") == LifeState::unconfigured);
  RC_CHECK(g.state_of("controller") == LifeState::unconfigured);
}

RC_TEST("when a required node cannot activate, the nodes already running are stopped and everything is given back, newest first") {
  Launcher g;
  g.add("planner", {"filter"}, true);
  g.add("controller", {"planner"}, true).on(Transition::activate, []() { return CallbackResult::failure; });
  g.add("filter", {"driver"}, true);
  g.add("driver", {}, true);
  const LaunchReport r = g.bring_up();
  std::cout << "  controller will not activate: failed '" << r.failed << "'\n  log: " << joined(r.log) << "\n\n";
  RC_CHECK(!r.ok);
  RC_CHECK(r.failed == "controller");
  RC_CHECK(joined(r.log) == "driver:configure filter:configure planner:configure controller:configure driver:activate filter:activate planner:activate controller:activate "
                            "controller:cleanup planner:deactivate planner:cleanup filter:deactivate filter:cleanup driver:deactivate driver:cleanup");
  RC_CHECK(g.state_of("driver") == LifeState::unconfigured);
  RC_CHECK(g.state_of("planner") == LifeState::unconfigured);
}

RC_TEST("an optional node that fails is left out with whatever needed it, and the rest of the graph comes up without it") {
  Launcher g;
  g.add("driver", {}, true);
  g.add("filter", {"driver"}, true);
  g.add("logger", {"driver"}, false).on(Transition::configure, []() { return CallbackResult::failure; });
  g.add("plotter", {"logger"}, false);
  g.add("controller", {"filter"}, true);
  const LaunchReport r = g.bring_up();
  std::cout << "  the logger will not configure: ok " << (r.ok ? "yes" : "no") << ", running: " << joined(r.running) << ", left out: " << joined(r.skipped) << "\n\n";
  RC_CHECK(r.ok);
  RC_CHECK(joined(r.running) == "driver filter controller");
  RC_CHECK(joined(r.skipped) == "logger plotter");
  RC_CHECK(g.state_of("controller") == LifeState::active);
  RC_CHECK(g.state_of("logger") == LifeState::unconfigured);
  RC_CHECK(g.state_of("plotter") == LifeState::unconfigured);

  // A required node that needs an optional one that failed can never have its need met, so the bring up stops.
  Launcher h;
  h.add("logger", {}, false).on(Transition::configure, []() { return CallbackResult::failure; });
  h.add("app", {"logger"}, true);
  const LaunchReport stopped = h.bring_up();
  std::cout << "  a required app that needs the failed optional logger: ok " << (stopped.ok ? "yes" : "no") << ", failed '" << stopped.failed << "'\n\n";
  RC_CHECK(!stopped.ok);
  RC_CHECK(stopped.failed == "app");
}
