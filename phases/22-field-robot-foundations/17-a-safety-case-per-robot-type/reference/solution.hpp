#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <algorithm>
#include <cstddef>
#include <set>
#include <sstream>
#include <string>
#include <vector>

enum class RobotKind { ground, arm, legged };

// How bad, how often and how avoidable, after the idea of a risk graph (not the standard's own tables): severity 1 slight, 2 serious, 3 death or lasting harm; exposure 1 rare, 2 frequent; avoidability 1 a person can get out of the way, 2 they cannot.
struct CaseHazard {
  std::string id;
  std::string description;
  int severity = 1;
  int exposure = 1;
  int avoidability = 1;
  std::string residual;    // "low", "medium" or "high": the risk that is left once the controls are in
  std::string signed_off;  // who accepts a high residual risk; empty if nobody has
};

struct CaseControl {
  std::string id;
  std::string hazard;      // the id of the hazard it is against
  std::string kind;        // "design" (remove it), "protective" (a safety function stops it), or "information" (a sign, a manual)
  std::string primitive;   // the safety primitive of the course that implements it, if one does
  std::string test;        // the test that proves it, a lesson or a test name; empty means nothing does
  bool tested = false;     // whether that test passes
};

struct SafetyCase {
  RobotKind kind = RobotKind::ground;
  std::string robot;
  std::vector<CaseHazard> hazards;
  std::vector<CaseControl> controls;
};

// "low", "medium" or "high" from the three ratings: the product is 1 to 12.
inline std::string risk_level(int severity, int exposure, int avoidability) {
  const int score = severity * exposure * avoidability;
  if (score <= 2) return "low";
  if (score <= 5) return "medium";
  return "high";
}

inline std::string risk_of(const CaseHazard& h) { return risk_level(h.severity, h.exposure, h.avoidability); }

// The safety primitives the course built in phase 19 and after, by the name a control uses for them.
inline const std::vector<std::string>& known_primitives() {
  static const std::vector<std::string> names = {
      "rc::safety::Supervisor", "rc::safety::StopPath", "rc::safety::Geofence",  "rc::safety::Limiter",   "rc::safety::CrossCheck", "rc::safety::TiltMonitor",
      "rc::safety::PowerMonitor", "rc::safety::SelfTest", "rc::safety::BlackBox", "rc::safety::Permit",   "rc::hal::watchdog",      "rc::field::limits"};
  return names;
}

inline CaseHazard make_hazard(const char* id, const char* text, int s, int e, int a) {
  CaseHazard h;
  h.id = id;
  h.description = text;
  h.severity = s;
  h.exposure = e;
  h.avoidability = a;
  return h;
}

// The hazards to start from for a kind of robot: no controls yet. A case is made by adding to these, not by starting from a blank page.
inline SafetyCase template_for(RobotKind kind) {
  SafetyCase c;
  c.kind = kind;
  if (kind == RobotKind::ground) {
    c.hazards.push_back(make_hazard("G1", "the vehicle collides with a person", 3, 2, 1));
    c.hazards.push_back(make_hazard("G2", "the vehicle keeps moving after it loses its command link", 3, 2, 2));
    c.hazards.push_back(make_hazard("G3", "the vehicle tips over on a slope or in a turn", 2, 2, 2));
    c.hazards.push_back(make_hazard("G4", "the vehicle cannot stop on a slope or on a loose surface", 3, 2, 2));
    c.hazards.push_back(make_hazard("G5", "the battery overheats or burns", 3, 1, 2));
    c.hazards.push_back(make_hazard("G6", "a limb is trapped between a wheel or track and the body", 2, 1, 2));
    c.hazards.push_back(make_hazard("G7", "a lost position takes the vehicle into a no go area", 2, 2, 1));
    c.hazards.push_back(make_hazard("G8", "the vehicle moves unexpectedly during maintenance", 3, 1, 2));
  } else if (kind == RobotKind::arm) {
    c.hazards.push_back(make_hazard("A1", "a person is crushed or pinched between a link and a fixed object", 3, 2, 2));
    c.hazards.push_back(make_hazard("A2", "the arm moves unexpectedly when it is powered up", 3, 1, 2));
    c.hazards.push_back(make_hazard("A3", "a payload is dropped", 2, 2, 1));
    c.hazards.push_back(make_hazard("A4", "the arm moves fast near a singularity", 2, 2, 2));
    c.hazards.push_back(make_hazard("A5", "a person is struck in the workspace", 3, 2, 1));
    c.hazards.push_back(make_hazard("A6", "a finger is trapped in the gripper", 2, 2, 2));
    c.hazards.push_back(make_hazard("A7", "the arm falls under its own weight when power is lost", 3, 1, 2));
  } else {
    c.hazards.push_back(make_hazard("L1", "the robot falls onto a person", 2, 2, 1));
    c.hazards.push_back(make_hazard("L2", "a servo overheats and burns a person", 2, 1, 1));
    c.hazards.push_back(make_hazard("L3", "a finger is trapped in a leg joint", 2, 2, 2));
    c.hazards.push_back(make_hazard("L4", "the robot thrashes on the ground after a fall", 2, 2, 1));
    c.hazards.push_back(make_hazard("L5", "the battery overheats or burns", 3, 1, 2));
    c.hazards.push_back(make_hazard("L6", "the robot stumbles on a step with a person close", 2, 2, 1));
  }
  return c;
}

inline bool is_known_primitive(const std::string& name) {
  const std::vector<std::string>& names = known_primitives();
  return std::find(names.begin(), names.end(), name) != names.end();
}

// Every hole in a case, in words. An empty list is a case with none.
inline std::vector<std::string> check_case(const SafetyCase& c) {
  std::vector<std::string> problems;
  std::set<std::string> hazard_ids;
  for (std::size_t i = 0; i < c.hazards.size(); ++i)
    if (!hazard_ids.insert(c.hazards[i].id).second) problems.push_back("hazard " + c.hazards[i].id + " is defined twice");
  std::set<std::string> control_ids;
  for (std::size_t i = 0; i < c.controls.size(); ++i) {
    const CaseControl& k = c.controls[i];
    if (!control_ids.insert(k.id).second) problems.push_back("control " + k.id + " is defined twice");
    if (hazard_ids.count(k.hazard) == 0) problems.push_back("control " + k.id + " is against hazard " + k.hazard + ", which does not exist");
    if (k.test.empty()) problems.push_back("control " + k.id + " has no test that proves it");
    else if (!k.tested) problems.push_back("control " + k.id + " names test " + k.test + ", which does not pass");
    if (!k.primitive.empty() && !is_known_primitive(k.primitive)) problems.push_back("control " + k.id + " names " + k.primitive + ", which is not a primitive of the course");
  }
  for (std::size_t i = 0; i < c.hazards.size(); ++i) {
    const CaseHazard& h = c.hazards[i];
    int any = 0;
    int protective = 0;
    for (std::size_t k = 0; k < c.controls.size(); ++k) {
      if (c.controls[k].hazard != h.id) continue;
      ++any;
      if (c.controls[k].kind == "protective" || c.controls[k].kind == "design") ++protective;
    }
    if (any == 0) problems.push_back("hazard " + h.id + " has no control");
    else if (risk_of(h) == "high" && protective == 0) problems.push_back("hazard " + h.id + " is a high risk and has only information as a control");
    if (h.residual == "high" && h.signed_off.empty()) problems.push_back("hazard " + h.id + " has a high residual risk and nobody has signed it off");
    if (h.residual.empty()) problems.push_back("hazard " + h.id + " has no residual risk stated");
  }
  return problems;
}

inline std::string to_markdown(const SafetyCase& c) {
  std::ostringstream o;
  o << "# Safety case: " << c.robot << "\n\n";
  o << "| Hazard | Description | Risk | Controls | Residual |\n|---|---|---|---|---|\n";
  for (std::size_t i = 0; i < c.hazards.size(); ++i) {
    const CaseHazard& h = c.hazards[i];
    std::string controls;
    for (std::size_t k = 0; k < c.controls.size(); ++k)
      if (c.controls[k].hazard == h.id) controls += (controls.empty() ? "" : ", ") + c.controls[k].id + " (" + c.controls[k].kind + ", test " + c.controls[k].test + ")";
    o << "| " << h.id << " | " << h.description << " | " << risk_of(h) << " | " << (controls.empty() ? "none" : controls) << " | " << (h.residual.empty() ? "not stated" : h.residual) << " |\n";
  }
  const std::vector<std::string> problems = check_case(c);
  o << "\n" << problems.size() << " open problems.\n";
  for (std::size_t i = 0; i < problems.size(); ++i) o << "- " << problems[i] << "\n";
  return o.str();
}


#endif
