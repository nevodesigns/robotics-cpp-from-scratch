#include <rc/test/rc_test.hpp>
#include <cmath>
#include <sstream>
#include <cstring>

#include <iostream>

#include "solution.hpp"
#include <iomanip>
#include <iostream>

// The numbers this lesson publishes: its page, its header and its quiz quote them, so each is held here as it was
// measured. A change that moves one by more than half a unit of its last printed digit fails the suite.
struct Pin {
  const char* key;
  double value;
};
static const Pin kPins[] = {
    {"n_ground", 8.0},
    {"n_arm", 7.0},
    {"n_legged", 6.0},
    {"empty_problems", 16.0},
    {"filled_problems", 0.0},
    {"holes", 6.0},
    {"report_lines", 13.0},
    {"ground_lines", 14.0},
};

static void rec(const char* key, double value, int digits = 3) {
  std::cout << "@" << key << " " << std::fixed << std::setprecision(digits) << value << std::endl;
  std::cout.unsetf(std::ios::floatfield);
  for (const Pin& pin : kPins) {
    if (std::strcmp(pin.key, key) != 0) continue;
    double allowed = 0.5;
    for (int i = 0; i < digits; ++i) allowed /= 10.0;
    if (!(std::fabs(value - pin.value) <= allowed)) {
      std::ostringstream message;
      message << "the published number " << key << " was " << pin.value << " and is now " << value;
      rc::test::record(__FILE__, __LINE__, message.str());
    }
  }
}


namespace {

// A control for every hazard: a protective one with a primitive and a test that passes, and a stated residual risk.
SafetyCase filled(RobotKind kind) {
  SafetyCase c = template_for(kind);
  c.robot = "test robot";
  for (std::size_t i = 0; i < c.hazards.size(); ++i) {
    CaseControl k;
    k.id = "C" + std::to_string(i + 1);
    k.hazard = c.hazards[i].id;
    k.kind = "protective";
    k.primitive = "rc::safety::Supervisor";
    k.test = "19-10";
    k.tested = true;
    c.controls.push_back(k);
    c.hazards[i].residual = "low";
  }
  return c;
}

bool mentions(const std::vector<std::string>& v, const std::string& what) {
  for (std::size_t i = 0; i < v.size(); ++i)
    if (v[i].find(what) != std::string::npos) return true;
  return false;
}

}  // namespace

RC_TEST("the risk level is the product of severity, exposure and avoidability") {
  RC_CHECK(risk_level(1, 1, 1) == "low");
  RC_CHECK(risk_level(1, 2, 1) == "low");
  RC_CHECK(risk_level(2, 1, 2) == "medium");
  RC_CHECK(risk_level(3, 1, 1) == "medium");
  RC_CHECK(risk_level(3, 2, 1) == "high");
  RC_CHECK(risk_level(3, 2, 2) == "high");
}

RC_TEST("each robot type starts from its own hazards, with unique ids and some of them high") {
  const RobotKind kinds[] = {RobotKind::ground, RobotKind::arm, RobotKind::legged};
  const char* prefix[] = {"G", "A", "L"};
  double counts[3] = {0, 0, 0};
  for (int k = 0; k < 3; ++k) {
    const SafetyCase c = template_for(kinds[k]);
    counts[k] = static_cast<double>(c.hazards.size());
    int high = 0;
    std::set<std::string> ids;
    for (std::size_t i = 0; i < c.hazards.size(); ++i) {
      ids.insert(c.hazards[i].id);
      RC_CHECK(c.hazards[i].id.compare(0, 1, prefix[k]) == 0);
      if (risk_of(c.hazards[i]) == "high") ++high;
    }
    RC_CHECK(ids.size() == c.hazards.size());
    RC_CHECK(high >= 2);
    RC_CHECK(c.controls.empty());
  }
  rec("n_ground", counts[0], 0);
  rec("n_arm", counts[1], 0);
  rec("n_legged", counts[2], 0);
  RC_CHECK(counts[0] == 8 && counts[1] == 7 && counts[2] == 6);
}

RC_TEST("a template with nothing filled in has a problem for every hazard, and a filled one has none") {
  const SafetyCase empty = template_for(RobotKind::ground);
  const std::vector<std::string> problems = check_case(empty);
  rec("empty_problems", static_cast<double>(problems.size()), 0);
  RC_CHECK(problems.size() == 2 * empty.hazards.size());  // no control, and no residual stated
  RC_CHECK(mentions(problems, "hazard G1 has no control"));

  const RobotKind kinds[] = {RobotKind::ground, RobotKind::arm, RobotKind::legged};
  for (int k = 0; k < 3; ++k) RC_CHECK(check_case(filled(kinds[k])).empty());
  rec("filled_problems", static_cast<double>(check_case(filled(RobotKind::ground)).size()), 0);
}

RC_TEST("five deliberate holes are each found, in words") {
  SafetyCase c = filled(RobotKind::ground);
  c.controls[0].test = "";                                  // nothing proves C1
  c.controls[1].tested = false;                             // C2's test fails
  c.controls[2].primitive = "rc::safety::MagicShield";      // not a primitive
  c.controls[3].kind = "information";                       // G4 is a high risk with only a sign
  c.hazards[4].residual = "high";                           // G5, and nobody has signed it
  CaseControl twice = c.controls[5];                        // C6 defined twice
  c.controls.push_back(twice);
  const std::vector<std::string> problems = check_case(c);
  for (std::size_t i = 0; i < problems.size(); ++i) std::cout << "  " << problems[i] << "\n";
  rec("holes", static_cast<double>(problems.size()), 0);
  RC_CHECK(mentions(problems, "control C1 has no test"));
  RC_CHECK(mentions(problems, "control C2 names test 19-10, which does not pass"));
  RC_CHECK(mentions(problems, "MagicShield"));
  RC_CHECK(mentions(problems, "hazard G4 is a high risk and has only information"));
  RC_CHECK(mentions(problems, "hazard G5 has a high residual risk and nobody has signed it off"));
  RC_CHECK(mentions(problems, "control C6 is defined twice"));
  RC_CHECK(problems.size() == 6);
  c.hazards[4].signed_off = "the safety officer";
  RC_CHECK(!mentions(check_case(c), "nobody has signed"));
  CaseControl lost = c.controls[6];
  lost.id = "C99";
  lost.hazard = "G99";
  c.controls.push_back(lost);
  RC_CHECK(mentions(check_case(c), "hazard G99, which does not exist"));
}

RC_TEST("the report is a table with a row for each hazard, and says how many problems are open") {
  const SafetyCase good = filled(RobotKind::arm);
  const std::string text = to_markdown(good);
  int lines = 0;
  for (std::size_t i = 0; i < text.size(); ++i) lines += text[i] == '\n' ? 1 : 0;
  rec("report_lines", static_cast<double>(lines), 0);
  RC_CHECK(text.find("# Safety case: test robot") == 0);
  RC_CHECK(text.find("| A1 |") != std::string::npos);
  RC_CHECK(text.find("| A7 |") != std::string::npos);
  RC_CHECK(text.find("0 open problems.") != std::string::npos);
  RC_CHECK(to_markdown(template_for(RobotKind::arm)).find("| A1 | a person is crushed or pinched between a link and a fixed object | high | none | not stated |") != std::string::npos);
  const std::string ground = to_markdown(filled(RobotKind::ground));
  int ground_lines = 0;
  for (std::size_t i = 0; i < ground.size(); ++i) ground_lines += ground[i] == '\n' ? 1 : 0;
  rec("ground_lines", static_cast<double>(ground_lines), 0);
}
