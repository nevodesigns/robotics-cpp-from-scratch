#include <rc/test/rc_test.hpp>

#include <iomanip>
#include <iostream>

#include "solution.hpp"

namespace {

using rc::capstone::DeliveryConfig;

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

DeliveryConfig slow_to_give_up() {
  DeliveryConfig c = good();
  c.forget_after = -1.0;
  return c;
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

std::string ids(const std::vector<std::string>& v) {
  std::string s;
  for (const std::string& x : v) s += (s.empty() ? "" : ", ") + x;
  return s.empty() ? "none" : s;
}

// The gates are the slow part (five runs of the rover each): each candidate is run once.
const GateResult& gate_good() {
  static const GateResult g = run_gate(good());
  return g;
}
const GateResult& gate_unsafe() {
  static const GateResult g = run_gate(unsafe());
  return g;
}
const GateResult& gate_slow() {
  static const GateResult g = run_gate(slow_to_give_up());
  return g;
}

}  // namespace

RC_TEST("a configuration is written as text one setting to a line, and a different configuration is a different release") {
  const std::string text = serialise_config(good());
  std::cout << text << "\n";
  RC_CHECK(text == "top_speed=0.8\nrobot_radius=0.15\ninflate=0.3\nlookahead=0.5\nuse_limiter=1\nuse_supervisor=1\nreplan_every=2\nforget_after=4\n");
  RC_CHECK(serialise_config(unsafe()).find("use_limiter=0\nuse_supervisor=0") != std::string::npos);
  RC_CHECK(serialise_config(slow_to_give_up()).find("forget_after=-1") != std::string::npos);
  const rc::deploy::Release a = rover_release("1.5.0", good(), "aarch64-linux-gnu");
  const rc::deploy::Release b = rover_release("1.5.0", unsafe(), "aarch64-linux-gnu");
  RC_CHECK(a.manifest_text != b.manifest_text);                     // the policy is in the manifest
  const rc::deploy::ParseResult p = rc::deploy::parse_manifest(a.manifest_text);
  RC_CHECK(p.ok && p.manifest.version == "1.5.0" && p.manifest.entries.size() == 2);
  RC_CHECK(rc::deploy::verify_files(p.manifest, a.files).empty());
  RC_CHECK(a.size_mb == 100.0);
}

RC_TEST("a good candidate passes the gate, goes through the pipeline of phase 20, and is the rover on the robot") {
  rc::deploy::RobotState r = robot();
  const ShipResult s = ship_rover(r, good(), rover_release("1.5.0", good(), "aarch64-linux-gnu"), false);
  std::cout << s.gate_report << "\n";
  std::cout << "  shipped: stages";
  for (const rc::deploy::StageResult& g : s.outcome.stages) std::cout << " " << g.stage;
  std::cout << "; running " << s.outcome.running_version << ", confirmed " << (s.outcome.confirmed ? "yes" : "no") << "\n\n";
  RC_CHECK(s.gate_passed && s.deployed);
  RC_CHECK(s.outcome.failed_stage.empty());
  RC_CHECK(s.outcome.running_version == "1.5.0" && s.outcome.confirmed);
  RC_CHECK(r.installed_version == "1.5.0");
  RC_CHECK(s.gate_report.find("Summary: 5 of 5 scenarios passed") != std::string::npos);
}

RC_TEST("a candidate that works in the empty building and has no safety layer fails the gate, and the robot is not touched") {
  rc::deploy::RobotState r = robot();
  const rc::deploy::RobotState before = robot();
  const ShipResult s = ship_rover(r, unsafe(), rover_release("1.5.0", unsafe(), "aarch64-linux-gnu"), false);
  const GateResult& g = gate_unsafe();
  std::cout << "  limiter and supervisor out: failing scenarios " << ids(g.failing_scenarios) << "; failing hazards " << ids(g.failing_hazards) << "; deployed " << (s.deployed ? "yes" : "NO") << "\n";
  std::cout << "  S1, the empty building, passed: " << (g.failing_scenarios.size() == 3 ? "yes (only the three safety scenarios failed)" : "no") << "\n\n";
  RC_CHECK(!s.gate_passed && !s.deployed);
  RC_CHECK(ids(g.failing_scenarios) == "S2, S4, S5");
  RC_CHECK(ids(g.failing_hazards) == "G1, G2, G4, G8");
  RC_CHECK(s.outcome.stages.empty());
  RC_CHECK(r.installed_version == before.installed_version);
  RC_CHECK(r.disk.boot_pointer == before.disk.boot_pointer && r.disk.slot[1].chunks.empty());
  RC_CHECK(s.gate_report.find("failing: G1, G2, G4, G8") != std::string::npos);
}

RC_TEST("a candidate that is safe and never gives up on a ghost in the doorway fails on a scenario that is for no hazard, and the gate is still red") {
  const GateResult& g = gate_slow();
  std::cout << "  forgets nothing: failing scenarios " << ids(g.failing_scenarios) << "; failing hazards " << ids(g.failing_hazards) << "; the gate " << (g.passed ? "passes" : "refuses") << "\n\n";
  RC_CHECK(!g.passed);
  RC_CHECK(ids(g.failing_scenarios) == "S3");
  RC_CHECK(g.failing_hazards.empty());                              // no hazard of the log is behind S3, and the gate does not need one
  RC_CHECK(g.report.find("S3  FAIL") != std::string::npos);
  RC_CHECK(gate_good().passed);
}

RC_TEST("a candidate that passes the gate and crashes on the robot is taken back by the trial, which is another layer and a different test") {
  rc::deploy::RobotState r = robot();
  rc::deploy::Release release = rover_release("1.5.0", good(), "aarch64-linux-gnu");
  release.behaviour.crash_at = 2.0;
  release.behaviour.healthy_at = -1.0;
  const ShipResult s = ship_rover(r, good(), release, false);
  RC_CHECK(!s.outcome.stages.empty());
  if (s.outcome.stages.empty()) return;
  std::cout << "  the gate " << (s.gate_passed ? "passed" : "refused") << ", the release went to the robot and then " << s.outcome.failed_stage << " failed (" << s.outcome.stages.back().detail << "); running " << s.outcome.running_version << "\n\n";
  RC_CHECK(s.gate_passed && s.deployed);
  RC_CHECK(s.outcome.failed_stage == "trial" && s.outcome.rolled_back);
  RC_CHECK(s.outcome.running_version == "1.4.2");
  RC_CHECK(r.installed_version == "1.4.2");
}

RC_TEST("across twenty robots a candidate with no safety layer reaches all of them without the gate and none with it") {
  int without_gate = 0;
  int with_gate = 0;
  const rc::deploy::Release bad = rover_release("1.5.0", unsafe(), "aarch64-linux-gnu");
  for (int i = 0; i < 20; ++i) {
    rc::deploy::RobotState a = robot();
    const rc::deploy::Outcome o = rc::deploy::deploy(a, bad, -1, false);
    if (o.failed_stage.empty() && o.running_version == "1.5.0") ++without_gate;
    rc::deploy::RobotState b = robot();
    if (!gate_unsafe().passed) continue;
    const ShipResult s = ship_rover(b, unsafe(), bad, false);
    if (s.deployed && s.outcome.running_version == "1.5.0") ++with_gate;
  }
  std::cout << "  twenty robots offered a rover with the limiter and supervisor taken out: " << without_gate << " run it with no gate, " << with_gate << " with the gate\n\n";
  RC_CHECK(without_gate == 20);
  RC_CHECK(with_gate == 0);
}
