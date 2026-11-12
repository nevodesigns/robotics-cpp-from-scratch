#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <iomanip>
#include <rc/capstone/delivery_rover.hpp>
#include <rc/capstone/test_report.hpp>
#include <rc/deploy/bundle.hpp>
#include <rc/deploy/release.hpp>
#include <rc/safety/hazards.hpp>
#include <rc/safety/supervisor.hpp>
#include <sstream>
#include <string>
#include <vector>

// The hazards of lesson 19-01.
inline std::vector<rc::safety::Hazard> rover_hazards() {
  const struct {
    const char* id;
    const char* what;
    int s;
    int l;
  } h[] = {{"G1", "runs into a person", 4, 3}, {"G2", "keeps driving when the link is lost", 4, 4}, {"G3", "rolls over on a slope", 3, 3}, {"G4", "cannot stop in the distance its sensor sees", 4, 3},
           {"G5", "battery fails in the field", 2, 4}, {"G6", "motor overheats and burns", 3, 2}, {"G7", "leaves the area it is allowed in", 3, 3}, {"G8", "a sensor fails and nothing notices", 3, 4}};
  std::vector<rc::safety::Hazard> v;
  for (const auto& x : h) {
    rc::safety::Hazard hz;
    hz.id = x.id;
    hz.description = x.what;
    hz.severity = x.s;
    hz.likelihood = x.l;
    hz.owner = "safety lead";
    v.push_back(hz);
  }
  return v;
}

// The five scenarios of lesson 21-08, with a candidate's configuration as the rover's: an empty building; somebody who steps into the path (evidence for G1 and G4); somebody in the doorway who then goes; the link lost for four seconds (G2); the range sensor dead as somebody steps out (G8).
// The configuration is the candidate's in every one, except for what the scenario itself sets (the link or the sensor).
inline std::vector<rc::capstone::SuiteScenario> default_suite(const rc::capstone::DeliveryConfig& candidate) {
  using rc::capstone::DeliveryResult;
  rc::capstone::Person steps;
  steps.x = 4.2;
  steps.y = 4.2;
  steps.x_end = 4.2;
  steps.y_end = 2.2;
  steps.radius = 0.25;
  steps.from = 3.0;
  steps.arrive = 4.2;
  steps.to = 12.0;
  rc::capstone::Person doorway;
  doorway.x = 7.2;
  doorway.y = 3.0;
  doorway.x_end = 5.1;
  doorway.y_end = 3.0;
  doorway.radius = 0.3;
  doorway.from = 2.0;
  doorway.arrive = 6.0;
  doorway.to = 20.0;
  std::vector<rc::capstone::SuiteScenario> s(5);
  s[0].id = "S1";
  s[0].title = "an empty building";
  s[0].passes = [](const DeliveryResult& r) { return r.reached && r.bumps == 0 && r.min_gap > 0.1; };
  s[1].id = "S2";
  s[1].title = "somebody steps into the path";
  s[1].hazards = {"G1", "G4"};
  s[1].people = {steps};
  s[1].passes = [](const DeliveryResult& r) { return r.reached && r.bumps == 0 && r.min_gap > 0.1; };
  s[2].id = "S3";
  s[2].title = "somebody in the doorway, then gone";
  s[2].people = {doorway};
  s[2].passes = [](const DeliveryResult& r) { return r.reached && r.bumps == 0; };
  s[3].id = "S4";
  s[3].title = "the link lost for four seconds";
  s[3].hazards = {"G2"};
  s[3].passes = [](const DeliveryResult& r) { return !r.reached && r.causes == rc::safety::cause_link && !r.moving_at_end; };
  s[4].id = "S5";
  s[4].title = "the range sensor dies";
  s[4].hazards = {"G8"};
  s[4].people = {steps};
  s[4].passes = [](const DeliveryResult& r) { return !r.reached && r.causes == rc::safety::cause_sensors && r.bumps == 0; };
  for (rc::capstone::SuiteScenario& x : s) x.config = candidate;
  s[3].config.link_lost_at = 5.0;
  s[3].config.link_back_at = 9.0;
  s[4].config.sensor_dead_at = 3.0;
  return s;
}

struct GateResult {
  bool passed = false;
  std::vector<std::string> failing_scenarios;      // the ids of the scenarios that failed
  std::vector<std::string> failing_hazards;        // the ids of the hazards that have a failing scenario
  std::string report;                              // the report of lesson 21-08
};

// The gate a candidate has to pass to be shipped: the five scenarios, run with its configuration, the report made, and passed only if rc::capstone::suite_green (no scenario failed, no hazard with a failing scenario).
inline GateResult run_gate(const rc::capstone::DeliveryConfig& candidate) {
  GateResult g;
  const std::vector<rc::capstone::SuiteScenario> scenarios = default_suite(candidate);
  const std::vector<rc::capstone::SuiteRow> rows = rc::capstone::run_suite(scenarios);
  const std::vector<rc::capstone::HazardLine> coverage = rc::capstone::hazard_coverage(rover_hazards(), scenarios, rows);
  for (const rc::capstone::SuiteRow& r : rows)
    if (!r.passed) g.failing_scenarios.push_back(r.id);
  for (const rc::capstone::HazardLine& h : coverage)
    if (h.status == rc::capstone::CoverageStatus::failing) g.failing_hazards.push_back(h.id);
  g.report = rc::capstone::render_report(scenarios, rows, coverage);
  g.passed = rc::capstone::suite_green(rows, coverage);
  return g;
}

// A configuration as text, one setting to a line as name=value, in this order, every line ending in a newline and the numbers written with std::ostringstream's default format:
// top_speed, robot_radius, inflate, lookahead, use_limiter (1 or 0), use_supervisor (1 or 0), replan_every, forget_after.
inline std::string serialise_config(const rc::capstone::DeliveryConfig& c) {
  // TODO: one name=value line each, in the order of the comment, with std::ostringstream's default number format and 1 or 0 for the two switches
  (void)c;
  return std::string();
}

// A release of the rover for a target: two files, "bin/rover" (a stand in for the program: the text "rover " and the version) and "etc/policy" (the configuration as text), a manifest of them, an image of four chunks of the program and the policy, a size of 100 MB and a behaviour of healthy at 20 s.
inline rc::deploy::Release rover_release(const std::string& version, const rc::capstone::DeliveryConfig& config, const std::string& triple) {
  rc::deploy::Release r;
  r.version = version;
  r.triple = triple;
  r.files = {{"bin/rover", "rover " + version}, {"etc/policy", serialise_config(config)}};
  r.manifest_text = rc::deploy::make_manifest(version, r.files).text;
  r.image_chunks = {"rover " + version, serialise_config(config), "chunk 3 of " + version, "chunk 4 of " + version};
  r.size_mb = 100.0;
  r.behaviour.healthy_at = 20.0;
  return r;
}

struct ShipResult {
  bool gate_passed = false;
  std::string gate_report;
  bool deployed = false;                          // the release went through the pipeline of lesson 20-12 (whatever its end)
  rc::deploy::Outcome outcome;
};

// Ship a candidate: run the gate first and, only if it passes, take the release to the robot with rc::deploy::deploy (no power cut, the rollback flag as given). A candidate that fails the gate is not deployed: the robot is not touched, the outcome is empty and `deployed` is false.
// There is no way to skip the gate.
inline ShipResult ship_rover(rc::deploy::RobotState& robot, const rc::capstone::DeliveryConfig& candidate, const rc::deploy::Release& release, bool rollback_authorised) {
  // TODO: run_gate on the candidate, keep its report and whether it passed; if it did not, return at once without touching the robot; otherwise rc::deploy::deploy(robot, release, -1, rollback_authorised), set deployed and keep the outcome
  (void)robot;
  (void)candidate;
  (void)release;
  (void)rollback_authorised;
  return ShipResult();
}

#endif  // LESSON_SOLUTION_HPP
