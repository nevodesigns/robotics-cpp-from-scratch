#include <rc/test/rc_test.hpp>
#include <rc/ros/launch.hpp>
#include <rc/ros/node.hpp>

#include <iomanip>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

#include "solution.hpp"

namespace {

using rc::ros::CallbackResult;
using rc::ros::InProcessNode;
using rc::ros::Launcher;
using rc::ros::World;

const char* kind_name(SupervisorEvent::Kind k) {
  switch (k) {
    case SupervisorEvent::Kind::stalled: return "stalled";
    case SupervisorEvent::Kind::starved: return "starved";
    case SupervisorEvent::Kind::restarted: return "restarted";
    case SupervisorEvent::Kind::restart_failed: return "restart_failed";
    case SupervisorEvent::Kind::gave_up: return "gave_up";
  }
  return "?";
}

// A robot's graph: a sensor driver, a filter, a planner and a controller, in a chain on the bus, each a managed node, with a viewer on the side.
// The controller commands the arm. It has a watchdog of its own: if no plan has come for 0.2 s it commands a stop, once.
struct Rig {
  World world;
  Launcher launcher;
  Supervisor supervisor;
  InProcessNode sensor_node{world, "/robot", "sensor"};
  InProcessNode filter_node{world, "/robot", "filter"};
  InProcessNode planner_node{world, "/robot", "planner"};
  InProcessNode controller_node{world, "/robot", "controller"};
  InProcessNode viewer_node{world, "/robot", "viewer"};
  InProcessNode probe{world, "/robot", "probe"};
  LifecycleNode* sensor = nullptr;
  LifecycleNode* viewer = nullptr;
  bool device_ok = true;             // whether the sensor can be opened
  bool sensor_stalled = false;       // the sensor is active and has stopped producing
  bool viewer_stalled = false;
  bool viewer_openable = true;
  int scans = 0;
  double last_scan = 0.0;
  int stops = 0;
  std::vector<std::pair<double, std::string>> commands;
  std::vector<std::pair<double, SupervisorEvent>> events;
  DeadlineMonitor watchdog{0.2};

  explicit Rig(int max_restarts) : supervisor(max_restarts) {
    sensor = &launcher.add("sensor", {}, true);
    LifecycleNode* filter = &launcher.add("filter", {"sensor"}, true);
    LifecycleNode* planner = &launcher.add("planner", {"filter"}, true);
    LifecycleNode* controller = &launcher.add("controller", {"planner"}, true);
    viewer = &launcher.add("viewer", {"planner"}, false);

    sensor->on(Transition::configure, [this]() {
      if (!device_ok) return CallbackResult::failure;
      sensor_stalled = false;                      // opening the device again clears the fault
      return CallbackResult::success;
    });
    viewer->on(Transition::configure, [this]() {
      if (!viewer_openable) return CallbackResult::failure;
      viewer_stalled = false;
      return CallbackResult::success;
    });

    auto scan_pub = sensor_node.create_publisher("scan");
    sensor_node.create_timer(0.1, [this, scan_pub]() {
      if (sensor_stalled) return;
      sensor->publish(std::to_string(scans++), scan_pub);
    });
    auto filtered_pub = filter_node.create_publisher("filtered");
    filter_node.create_subscription("scan", [filter, filtered_pub](const std::string& p) { filter->publish(p, filtered_pub); });
    auto plan_pub = planner_node.create_publisher("plan");
    planner_node.create_subscription("filtered", [planner, plan_pub](const std::string& p) { planner->publish(p, plan_pub); });
    auto cmd_pub = controller_node.create_publisher("cmd");
    controller_node.create_subscription("plan", [this, controller, cmd_pub](const std::string& p) {
      watchdog.on_message(world.clock.now());
      controller->publish("move " + p, cmd_pub);
    });
    controller_node.create_timer(0.01, [this, cmd_pub]() {
      if (watchdog.check(world.clock.now()) == 1) {
        ++stops;
        cmd_pub("stop");
      }
    });
    auto view_pub = viewer_node.create_publisher("picture");
    viewer_node.create_subscription("plan", [this, view_pub](const std::string& p) {
      if (!viewer_stalled) viewer->publish(p, view_pub);
    });
    // What the supervisor sees, and what the arm is told.
    probe.create_subscription("/robot/scan", [this](const std::string&) {
      last_scan = world.clock.now();
      supervisor.on_output("sensor", world.clock.now());
    });
    probe.create_subscription("/robot/picture", [this](const std::string&) { supervisor.on_output("viewer", world.clock.now()); });
    probe.create_subscription("/robot/cmd", [this](const std::string& p) { commands.push_back({world.clock.now(), p}); });

    supervisor.watch("sensor", sensor, 0.25, true, {});
    supervisor.watch("viewer", viewer, 0.25, false, {"sensor"});     // the viewer is fed from the sensor, through the filter and planner
  }

  // Run for `seconds` in steps of 10 ms. The supervisor looks after every step.
  void run(double seconds) {
    const int steps = static_cast<int>(seconds / 0.01 + 0.5);
    for (int i = 0; i < steps; ++i) {
      world.advance(0.01);
      for (const SupervisorEvent& e : supervisor.tick(world.clock.now())) events.push_back({world.clock.now(), e});
    }
  }

  int moves() const {
    int n = 0;
    for (const auto& c : commands)
      if (c.second != "stop") ++n;
    return n;
  }
  double first_move_after(double t) const {
    for (const auto& c : commands)
      if (c.first > t && c.second != "stop") return c.first;
    return -1.0;
  }
  double first_stop() const {
    for (const auto& c : commands)
      if (c.second == "stop") return c.first;
    return -1.0;
  }
};

}  // namespace

RC_TEST("a healthy graph is brought up and commands the arm and gives the supervisor nothing to do, and the same graph with its sensor stopped does") {
  Rig rig(2);
  const rc::ros::LaunchReport up = rig.launcher.bring_up();
  RC_CHECK(up.ok);
  RC_CHECK(up.running.size() == 5);
  rig.run(10.0);
  std::cout << "  10 s of a healthy graph: " << rig.moves() << " move commands, " << rig.stops << " stops, " << rig.events.size() << " supervisor events\n\n";
  RC_CHECK(rig.moves() >= 95);
  RC_CHECK(rig.moves() <= 101);
  RC_CHECK(rig.stops == 0);
  RC_CHECK(rig.events.empty());
  rig.sensor_stalled = true;
  rig.run(1.0);
  std::cout << "  and with the sensor stopped for a second: " << rig.events.size() << " supervisor events\n\n";
  RC_CHECK(!rig.events.empty());
}

RC_TEST("a sensor that stalls is noticed within its deadline, restarted through its life cycle, and the arm is stopped meanwhile and moves again after") {
  Rig rig(2);
  rig.launcher.bring_up();
  rig.run(5.0);
  const int before = rig.scans;
  const double last_scan_before = rig.last_scan;
  rig.sensor_stalled = true;                              // the driver is still active and has stopped producing
  const double stall_at = rig.world.clock.now();
  rig.run(3.0);
  double detected = -1.0;
  double restarted = -1.0;
  for (const auto& e : rig.events) {
    if (e.second.kind == SupervisorEvent::Kind::stalled && detected < 0.0) detected = e.first;
    if (e.second.kind == SupervisorEvent::Kind::restarted && restarted < 0.0) restarted = e.first;
  }
  const double stop_at = rig.first_stop();
  const double moving_again = rig.first_move_after(stall_at + 0.05);
  int viewer_starved = 0;
  for (const auto& e : rig.events)
    if (e.second.node == "viewer" && e.second.kind == SupervisorEvent::Kind::starved) ++viewer_starved;
  std::cout << "  the sensor stalled at " << std::fixed << std::setprecision(2) << stall_at << " s (" << before << " scans so far, the last at " << last_scan_before << ")\n"
            << "  the arm was told to stop at " << stop_at << " s (the controller's watchdog, 0.20 s)\n"
            << "  the supervisor saw the sensor stalled at " << detected << " s and restarted it at " << restarted << " s\n"
            << "  the arm moved again at " << moving_again << " s; stops commanded: " << rig.stops << "; restarts of the sensor: " << rig.supervisor.restarts("sensor")
            << "\n  the viewer, fed from the sensor, was also silent: reported starved " << viewer_starved << " time, restarted " << rig.supervisor.restarts("viewer") << " times\n\n";
  RC_CHECK(detected > last_scan_before + 0.25);                        // one deadline of silence, 0.25 s, after the last scan
  RC_CHECK(detected < last_scan_before + 0.30);
  RC_CHECK(viewer_starved == 1);
  RC_CHECK(rig.supervisor.restarts("viewer") == 0);
  RC_CHECK(restarted >= detected);
  RC_CHECK(stop_at > stall_at);
  RC_CHECK(stop_at < detected);                           // the arm was stopped before the supervisor acted
  RC_CHECK(rig.stops == 1);
  RC_CHECK(moving_again > restarted);
  RC_CHECK(moving_again < restarted + 0.2);
  RC_CHECK(rig.supervisor.restarts("sensor") == 1);
  RC_CHECK(!rig.supervisor.gave_up());
  RC_CHECK(rig.sensor->state() == LifeState::active);
  // No move command was issued in the silence: between the stop and the moving again there is nothing but the stop.
  int moves_in_silence = 0;
  for (const auto& c : rig.commands)
    if (c.first > stop_at && c.first < moving_again && c.second != "stop") ++moves_in_silence;
  RC_CHECK(moves_in_silence == 0);
}

RC_TEST("a sensor that cannot be restarted is restarted the allowed number of times, then given up on, and the graph is taken down in reverse") {
  Rig rig(2);
  rig.launcher.bring_up();
  rig.run(5.0);
  rig.sensor_stalled = true;
  rig.device_ok = false;                                  // the cable is out: configure will fail
  const double stall_at = rig.world.clock.now();
  for (int i = 0; i < 300 && !rig.supervisor.gave_up(); ++i) rig.run(0.01);
  RC_CHECK(rig.supervisor.gave_up());
  std::vector<std::pair<double, SupervisorEvent>> sensor_events;
  int viewer_starved = 0;
  int viewer_other = 0;
  for (const auto& e : rig.events) {
    if (e.second.node == "sensor") sensor_events.push_back(e);
    if (e.second.node == "viewer") {
      if (e.second.kind == SupervisorEvent::Kind::starved) ++viewer_starved;
      else ++viewer_other;
    }
  }
  std::cout << "  the sensor stalled at " << std::fixed << std::setprecision(2) << stall_at << " s and its device cannot be opened\n  the sensor's events:";
  for (const auto& e : sensor_events) std::cout << "  " << e.first << " " << kind_name(e.second.kind) << ";";
  const double gave_up_at = sensor_events.empty() ? -1.0 : sensor_events.back().first;
  const rc::ros::LaunchReport down = rig.launcher.shut_down();
  std::cout << "\n  the viewer was starved " << viewer_starved << " times and restarted " << rig.supervisor.restarts("viewer") << "; gave up " << gave_up_at - stall_at
            << " s after the stall; the graph was then taken down: " << down.log.size() << " transitions; the arm was told to stop " << rig.stops << " time(s)\n\n";
  RC_CHECK(sensor_events.size() == 6);
  if (sensor_events.size() != 6) return;
  RC_CHECK(sensor_events[0].second.kind == SupervisorEvent::Kind::stalled);
  RC_CHECK(sensor_events[1].second.kind == SupervisorEvent::Kind::restart_failed);
  RC_CHECK(sensor_events[2].second.kind == SupervisorEvent::Kind::stalled);
  RC_CHECK(sensor_events[3].second.kind == SupervisorEvent::Kind::restart_failed);
  RC_CHECK(sensor_events[4].second.kind == SupervisorEvent::Kind::stalled);
  RC_CHECK(sensor_events[5].second.kind == SupervisorEvent::Kind::gave_up);
  RC_CHECK(rig.supervisor.restarts("sensor") == 2);
  RC_CHECK(viewer_starved >= 3);
  RC_CHECK(viewer_other == 0);
  RC_CHECK(rig.supervisor.restarts("viewer") == 0);
  RC_CHECK(gave_up_at - stall_at > 0.6);
  RC_CHECK(gave_up_at - stall_at < 1.0);
  RC_CHECK(rig.launcher.state_of("controller") == LifeState::unconfigured);
  RC_CHECK(rig.launcher.state_of("sensor") == LifeState::unconfigured);
  RC_CHECK(rig.stops == 1);
}

RC_TEST("an optional node that stalls and cannot be restarted is given up on without stopping the graph") {
  Rig rig(1);
  rig.launcher.bring_up();
  rig.run(2.0);
  rig.viewer_stalled = true;
  rig.viewer_openable = false;
  rig.run(3.0);
  int gave_up_events = 0;
  for (const auto& e : rig.events)
    if (e.second.kind == SupervisorEvent::Kind::gave_up && e.second.node == "viewer") ++gave_up_events;
  const int moves_total = rig.moves();
  std::cout << "  the viewer stalled and could not be reopened: given up on " << gave_up_events << " time, graph gave_up() is " << (rig.supervisor.gave_up() ? "true" : "false")
            << ", the arm still got " << moves_total << " move commands in 5 s, " << rig.stops << " stops\n\n";
  RC_CHECK(gave_up_events == 1);
  RC_CHECK(!rig.supervisor.gave_up());
  RC_CHECK(moves_total >= 45);
  RC_CHECK(rig.stops == 0);
  RC_CHECK(rig.sensor->state() == LifeState::active);
}

RC_TEST("a restart of a node that works gives it a fresh deadline, so it is not reported again at once, and a node that is never silent is never restarted") {
  Rig rig(3);
  rig.launcher.bring_up();
  rig.run(4.0);
  rig.sensor_stalled = true;
  rig.run(2.0);
  RC_CHECK(rig.supervisor.restarts("sensor") == 1);                      // restarted once and then it ran
  int stalls = 0;
  for (const auto& e : rig.events)
    if (e.second.kind == SupervisorEvent::Kind::stalled) ++stalls;
  std::cout << "  one stall in 6 s: " << stalls << " stalled event, " << rig.supervisor.restarts("sensor") << " restart, viewer restarts " << rig.supervisor.restarts("viewer") << "\n\n";
  RC_CHECK(stalls == 1);
  RC_CHECK(rig.supervisor.restarts("viewer") == 0);
}
