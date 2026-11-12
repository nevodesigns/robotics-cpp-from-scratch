#include <rc/test/rc_test.hpp>

#include <iomanip>
#include <iostream>

#include "solution.hpp"

namespace {

using rc::capstone::DeliveryConfig;
using rc::capstone::DeliveryResult;
using rc::capstone::SuiteRow;

DeliveryConfig good() {
  DeliveryConfig c;
  c.inflate = 0.3;
  return c;
}

DeliveryConfig unsafe() {
  DeliveryConfig c = good();
  c.use_limiter = false;
  c.use_supervisor = false;
  return c;
}

const DayResult& good_day() {
  static const DayResult d = run_day(good(), "1.5.0", true);
  return d;
}

const DayResult& unsafe_day() {
  static const DayResult d = run_day(unsafe(), "1.5.0-unsafe", false);
  return d;
}

rc::deploy::RobotState robot() {
  rc::deploy::RobotState r;
  r.triple = "aarch64-linux-gnu";
  r.installed_version = "1.4.2";
  r.minimum_version = "1.3.0";
  r.disk.slot[0].chunks = {"rover 1.4.2", "old policy", "chunk 3 of 1.4.2", "chunk 4 of 1.4.2"};
  r.disk.slot[0].version = "1.4.2";
  r.disk.slot[0].stored_hash = rc::deploy::image_hash(r.disk.slot[0].chunks);
  r.board.cores = 4.0;
  r.board.ram_mb = 4096.0;
  r.board.flash_mb = 16384.0;
  r.data_mb = 2000.0;
  r.log_mb = 200.0;
  return r;
}

SuiteRow row(bool reached, unsigned causes) {
  SuiteRow r;
  r.result.reached = reached;
  r.result.causes = causes;
  return r;
}

}  // namespace

RC_TEST("a day's jobs are counted as delivered when they reach the goal and as stopped when the supervisor ended them") {
  const std::vector<SuiteRow> rows = {row(true, 0), row(true, 0), row(false, rc::safety::cause_link), row(false, rc::safety::cause_sensors), row(false, 0)};
  RC_CHECK(count_delivered(rows) == 2);
  RC_CHECK(count_stopped(rows) == 2);
  RC_CHECK(count_delivered({}) == 0 && count_stopped({}) == 0);
  RC_CHECK(count_delivered({row(true, rc::safety::cause_tilt)}) == 1);                 // a job can reach the goal and have been stopped on the way
  RC_CHECK(count_stopped({row(true, rc::safety::cause_tilt)}) == 1);
}

RC_TEST("the report of a day is a fixed five lines, which is what is handed to somebody who was not there") {
  DayResult d;
  d.version = "1.5.0";
  d.gate_passed = true;
  d.jobs = 5;
  d.delivered = 3;
  d.stopped = 2;
  d.bumps = 0;
  d.operator_resets = 2;
  d.causes_seen = rc::safety::cause_link | rc::safety::cause_sensors;
  d.log_events = 10;
  d.log_verified = true;
  d.before_first_stop = {{2, 12.3}, {1, 1.0}, {99, 4.0}};
  d.uncovered = {"G3", "G5"};
  const std::string text = render_day(d);
  std::cout << text << "\n";
  RC_CHECK(text ==
           "Rover 1.5.0 (gate passed)\n"
           "Day: 5 jobs, 3 delivered, 2 stopped by the supervisor (link, sensors), 0 bumps, 2 operator resets\n"
           "Black box: 10 events, chain verified\n"
           "Before the first stop: 2:12.3 1:1.0 99:4.0\n"
           "Open: hazards with no scenario: G3, G5\n");
  DayResult e;
  e.version = "1.5.0-unsafe";
  const std::string bad = render_day(e);
  RC_CHECK(bad.find("(gate REFUSED)") != std::string::npos);
  RC_CHECK(bad.find("(none)") != std::string::npos);
  RC_CHECK(bad.find("chain BROKEN") != std::string::npos);
  RC_CHECK(bad.find("Before the first stop: none\n") != std::string::npos);
  RC_CHECK(bad.find("Open: nothing uncovered\n") != std::string::npos);
}

RC_TEST("a day of the rover with its safety layer: three jobs delivered, two ended by a stop with its cause, nothing touched, and a black box that says how it went") {
  const DayResult& d = good_day();
  std::cout << d.report << "\n";
  RC_CHECK(d.jobs == 5);
  RC_CHECK(d.delivered == 3);
  RC_CHECK(d.stopped == 2);
  RC_CHECK(d.bumps == 0);
  RC_CHECK(d.operator_resets == 2);
  RC_CHECK(d.causes_seen == (rc::safety::cause_link | rc::safety::cause_sensors));
  RC_CHECK(d.log_events == 10);
  RC_CHECK(d.log_verified);
  RC_CHECK(d.uncovered.size() == 4 && d.uncovered[0] == "G3" && d.uncovered[3] == "G7");
  RC_CHECK(d.before_first_stop.size() == 6);
  if (d.before_first_stop.size() != 6) return;
  RC_CHECK(d.before_first_stop[0].first == 2 && d.before_first_stop[5].first == 1);   // job 3 starts and the stop follows
  RC_CHECK(d.before_first_stop[0].second > 12.0 && d.before_first_stop[0].second < 12.6);
  RC_CHECK(d.report.find("Day: 5 jobs, 3 delivered, 2 stopped by the supervisor (link, sensors), 0 bumps, 2 operator resets\n") != std::string::npos);
  RC_CHECK(d.report.find("Open: hazards with no scenario: G3, G5, G6, G7\n") != std::string::npos);
}

RC_TEST("the same day with the safety layer taken out delivers more and is the worse day by every other measure, and the gate would have said so") {
  const DayResult& a = good_day();
  const DayResult& b = unsafe_day();
  std::cout << b.report << "\n";
  std::cout << "  against the same day with the layer: delivered " << b.delivered << " against " << a.delivered << ", stopped " << b.stopped << " against " << a.stopped << ", bumps " << b.bumps << " against " << a.bumps << "\n\n";
  RC_CHECK(b.delivered > a.delivered);
  RC_CHECK(b.stopped == 0);
  RC_CHECK(b.bumps > 200);
  RC_CHECK(b.operator_resets == 0);
  RC_CHECK(!b.gate_passed);
  RC_CHECK(b.report.find("(gate REFUSED)") != std::string::npos);
  RC_CHECK(b.log_verified);                                           // the log is intact, and it records the contact: 3 is the code for bumps
}

RC_TEST("the release is shipped through the gate and the pipeline, the robot runs the day, and when the gate refuses, the robot's day is the day it had before") {
  rc::deploy::RobotState r = robot();
  const rc::deploy::Release release = rc::capstone::rover_release("1.5.0", good(), "aarch64-linux-gnu");
  const rc::capstone::ShipResult shipped = rc::capstone::ship_rover(r, good(), release, false);
  rc::deploy::RobotState q = robot();
  const rc::capstone::ShipResult refused = rc::capstone::ship_rover(q, unsafe(), rc::capstone::rover_release("1.5.0", unsafe(), "aarch64-linux-gnu"), false);
  std::cout << "  gate and pipeline for the good candidate: " << (shipped.gate_passed ? "passed" : "refused") << ", running " << shipped.outcome.running_version << "; for the unsafe one: " << (refused.gate_passed ? "passed" : "refused") << ", the robot still on " << q.installed_version << "\n";
  const DayResult before = run_day(good(), q.installed_version, refused.gate_passed);
  std::cout << "  the day on " << before.version << " after the refusal: " << before.delivered << " delivered, " << before.stopped << " stopped, " << before.bumps << " bumps\n\n";
  RC_CHECK(shipped.gate_passed && shipped.deployed && shipped.outcome.running_version == "1.5.0" && shipped.outcome.confirmed);
  RC_CHECK(!refused.gate_passed && !refused.deployed);
  RC_CHECK(q.installed_version == "1.4.2");
  RC_CHECK(before.delivered == good_day().delivered && before.stopped == good_day().stopped && before.bumps == 0);
}

RC_TEST("everything the report claims is something a test checked, and the open line says what none did") {
  const DayResult& d = good_day();
  RC_CHECK(d.report.find("Rover 1.5.0 (gate passed)") == 0);
  RC_CHECK(d.report.find("Black box: 10 events, chain verified") != std::string::npos);
  RC_CHECK(d.report.find("Before the first stop: 2:") != std::string::npos);
  RC_CHECK(!d.uncovered.empty());                                     // the report does not say that nothing is missing: four hazards have no scenario
  std::cout << "  the open line of the report lists " << d.uncovered.size() << " hazards with no scenario; the robot has not been shown to be safe against them, and the report says so\n\n";
}
