#include <rc/test/rc_test.hpp>

#include <iomanip>
#include <iostream>

#include "solution.hpp"

namespace {

using rc::capstone::DeliveryResult;
using rc::capstone::Person;

std::vector<rc::safety::Hazard> hazards() {
  const struct { const char* id; const char* what; int s; int l; } h[] = {
      {"G1", "runs into a person", 4, 3}, {"G2", "keeps driving when the link is lost", 4, 4}, {"G3", "rolls over on a slope", 3, 3}, {"G4", "cannot stop in the distance its sensor sees", 4, 3},
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

Person steps_out() {
  Person p;
  p.x = 4.2;
  p.y = 4.2;
  p.x_end = 4.2;
  p.y_end = 2.2;
  p.radius = 0.25;
  p.from = 3.0;
  p.arrive = 4.2;
  p.to = 12.0;
  return p;
}

Person in_the_doorway() {
  Person p;
  p.x = 7.2;
  p.y = 3.0;
  p.x_end = 5.1;
  p.y_end = 3.0;
  p.radius = 0.3;
  p.from = 2.0;
  p.arrive = 6.0;
  p.to = 20.0;
  return p;
}

rc::capstone::DeliveryConfig config() {
  rc::capstone::DeliveryConfig c;
  c.inflate = 0.3;
  return c;
}

std::vector<SuiteScenario> suite() {
  std::vector<SuiteScenario> s;
  {
    SuiteScenario x;
    x.id = "S1";
    x.title = "an empty building";
    x.config = config();
    x.passes = [](const DeliveryResult& r) { return r.reached && r.bumps == 0 && r.min_gap > 0.1; };
    s.push_back(x);
  }
  {
    SuiteScenario x;
    x.id = "S2";
    x.title = "somebody steps into the path";
    x.hazards = {"G1", "G4"};
    x.config = config();
    x.people = {steps_out()};
    x.passes = [](const DeliveryResult& r) { return r.reached && r.bumps == 0 && r.min_gap > 0.1; };
    s.push_back(x);
  }
  {
    SuiteScenario x;
    x.id = "S3";
    x.title = "somebody in the doorway, then gone";
    x.config = config();
    x.people = {in_the_doorway()};
    x.passes = [](const DeliveryResult& r) { return r.reached && r.bumps == 0; };
    s.push_back(x);
  }
  {
    SuiteScenario x;
    x.id = "S4";
    x.title = "the link lost for four seconds";
    x.hazards = {"G2"};
    x.config = config();
    x.config.link_lost_at = 5.0;
    x.config.link_back_at = 9.0;
    x.passes = [](const DeliveryResult& r) { return !r.reached && r.causes == rc::safety::cause_link && !r.moving_at_end; };
    s.push_back(x);
  }
  {
    SuiteScenario x;
    x.id = "S5";
    x.title = "the range sensor dies";
    x.hazards = {"G8"};
    x.config = config();
    x.config.sensor_dead_at = 3.0;
    x.people = {steps_out()};
    x.passes = [](const DeliveryResult& r) { return !r.reached && r.causes == rc::safety::cause_sensors && r.bumps == 0; };
    s.push_back(x);
  }
  return s;
}

// A result made by hand, so that the layout of the report can be tested without a simulation under it.
DeliveryResult made(bool reached, double time, int bumps, double gap, unsigned causes) {
  DeliveryResult r;
  r.reached = reached;
  r.time = time;
  r.bumps = bumps;
  r.min_gap = gap;
  r.causes = causes;
  return r;
}

}  // namespace

RC_TEST("a hazard is covered when a scenario names it and all of them pass, failing when one of them fails, and has no test when none names it, and low ones are left out") {
  const std::vector<SuiteScenario> s = suite();
  std::vector<SuiteRow> rows(5);
  for (std::size_t i = 0; i < 5; ++i) {
    rows[i].id = s[i].id;
    rows[i].passed = true;
  }
  std::vector<rc::safety::Hazard> h = hazards();
  rc::safety::Hazard low;
  low.id = "G9";
  low.description = "a scuffed bumper";
  low.severity = 1;
  low.likelihood = 2;
  low.owner = "ops";
  h.push_back(low);
  std::vector<HazardLine> c = hazard_coverage(h, s, rows);
  RC_CHECK(c.size() == 8);                                         // G9 is class low and left out
  if (c.size() != 8) return;
  RC_CHECK(c[0].id == "G1" && c[0].status == CoverageStatus::covered && c[0].scenarios.size() == 1 && c[0].scenarios[0] == "S2");
  RC_CHECK(c[1].id == "G2" && c[1].status == CoverageStatus::covered && c[1].risk == rc::safety::RiskClass::unacceptable);
  RC_CHECK(c[2].id == "G3" && c[2].status == CoverageStatus::uncovered && c[2].scenarios.empty());
  RC_CHECK(c[3].id == "G4" && c[3].status == CoverageStatus::covered);
  RC_CHECK(c[7].id == "G8" && c[7].status == CoverageStatus::covered);
  rows[1].passed = false;                                          // S2 fails: G1 and G4 are failing
  c = hazard_coverage(h, s, rows);
  RC_CHECK(c[0].status == CoverageStatus::failing && c[3].status == CoverageStatus::failing);
  RC_CHECK(c[1].status == CoverageStatus::covered);
  RC_CHECK(!suite_green(rows, c));
  std::vector<SuiteRow> good(5);
  for (SuiteRow& r : good) r.passed = true;
  const std::vector<HazardLine> gc = hazard_coverage(h, s, good);
  RC_CHECK(suite_green(good, gc));                                 // uncovered hazards are a gap and not a failure of what was tested
  std::cout << "  five scenarios against eight hazards: G1 and G4 by S2, G2 by S4, G8 by S5; G3, G5, G6 and G7 by none; a hazard of class low is left out\n\n";
}

RC_TEST("the report is a fixed layout, scenarios and then hazards and then a summary, with what failed and what is missing named in it") {
  std::vector<SuiteScenario> s = suite();
  std::vector<SuiteRow> rows(5);
  const DeliveryResult results[5] = {made(true, 12.3, 0, 0.18, 0), made(false, 120.0, 144, 0.02, 0), made(true, 27.2, 0, 0.34, 0), made(false, 120.0, 0, 0.46, rc::safety::cause_link), made(false, 120.0, 0, 0.48, rc::safety::cause_sensors)};
  const bool passed[5] = {true, false, true, true, true};
  for (std::size_t i = 0; i < 5; ++i) {
    rows[i].id = s[i].id;
    rows[i].passed = passed[i];
    rows[i].result = results[i];
  }
  const std::string text = render_report(s, rows, hazard_coverage(hazards(), s, rows));
  std::cout << text << "\n";
  const std::string want =
      "Scenarios\n"
      "S1  PASS  an empty building                   reached 12.3 s, 0 bumps, closest 0.18 m\n"
      "S2  FAIL  somebody steps into the path        not reached, 144 bumps\n"
      "S3  PASS  somebody in the doorway, then gone  reached 27.2 s, 0 bumps, closest 0.34 m\n"
      "S4  PASS  the link lost for four seconds      not reached, 0 bumps, stopped: link\n"
      "S5  PASS  the range sensor dies               not reached, 0 bumps, stopped: sensors\n"
      "\n"
      "Hazards of class medium or worse\n"
      "G1  high          runs into a person                            FAILING in S2\n"
      "G2  unacceptable  keeps driving when the link is lost           covered by S4\n"
      "G3  medium        rolls over on a slope                         NOT COVERED\n"
      "G4  high          cannot stop in the distance its sensor sees   FAILING in S2\n"
      "G5  medium        battery fails in the field                    NOT COVERED\n"
      "G6  medium        motor overheats and burns                     NOT COVERED\n"
      "G7  medium        leaves the area it is allowed in              NOT COVERED\n"
      "G8  high          a sensor fails and nothing notices            covered by S5\n"
      "\n"
      "Summary: 4 of 5 scenarios passed; 2 of 8 hazards covered by passing scenarios; not covered: G3, G5, G6, G7; failing: G1, G4\n";
  RC_CHECK(text == want);
}

RC_TEST("the rover with its safety layer on passes all five scenarios, and the report says that four of the eight hazards have no test at all") {
  const std::vector<SuiteScenario> s = suite();
  const std::vector<SuiteRow> rows = run_suite(s);
  const std::vector<HazardLine> c = hazard_coverage(hazards(), s, rows);
  const std::string text = render_report(s, rows, c);
  std::cout << text << "\n";
  RC_CHECK(rows.size() == 5);
  if (rows.size() != 5) return;
  for (const SuiteRow& r : rows) RC_CHECK(r.passed);
  RC_CHECK(rows[0].result.reached && rows[0].result.time > 12.0 && rows[0].result.time < 12.6);
  RC_CHECK(rows[3].result.causes == rc::safety::cause_link);
  RC_CHECK(rows[4].result.causes == rc::safety::cause_sensors);
  int uncovered = 0;
  for (const HazardLine& h : c)
    if (h.status == CoverageStatus::uncovered) ++uncovered;
  RC_CHECK(uncovered == 4);
  RC_CHECK(suite_green(rows, c));
  RC_CHECK(text.find("Summary: 5 of 5 scenarios passed; 4 of 8 hazards covered by passing scenarios; not covered: G3, G5, G6, G7\n") != std::string::npos);
}

RC_TEST("a change that takes the range limiter out still reaches the goal in an empty building, and is caught by the scenario with the person in it") {
  std::vector<SuiteScenario> s = suite();
  for (SuiteScenario& x : s) {
    x.config.use_limiter = false;
    x.config.use_supervisor = false;
  }
  const std::vector<SuiteRow> rows = run_suite(s);
  const std::vector<HazardLine> c = hazard_coverage(hazards(), s, rows);
  std::cout << "  limiter and supervisor taken out:\n";
  for (const SuiteRow& r : rows) std::cout << "  " << r.id << (r.passed ? "  PASS" : "  FAIL") << "\n";
  std::cout << "\n";
  RC_CHECK(rows.size() == 5);
  if (rows.size() != 5) return;
  RC_CHECK(rows[0].passed);                                        // the empty building: nothing to see
  RC_CHECK(!rows[1].passed);                                       // the person: contact
  RC_CHECK(!rows[3].passed);                                       // the link: nobody listens
  RC_CHECK(!rows[4].passed);
  RC_CHECK(!suite_green(rows, c));
  RC_CHECK(c.size() == 8);
  if (c.size() != 8) return;
  RC_CHECK(c[0].status == CoverageStatus::failing && c[1].status == CoverageStatus::failing);
  const std::string text = render_report(s, rows, c);
  RC_CHECK(text.find("failing: G1, G2, G4, G8") != std::string::npos);
}

RC_TEST("a suite with the link scenario left out still passes, and the report puts the unacceptable hazard in the list of what is not covered") {
  std::vector<SuiteScenario> s = suite();
  s.erase(s.begin() + 3);
  const std::vector<SuiteRow> rows = run_suite(s);
  const std::vector<HazardLine> c = hazard_coverage(hazards(), s, rows);
  const std::string text = render_report(s, rows, c);
  RC_CHECK(rows.size() == 4);
  RC_CHECK(suite_green(rows, c));                                  // every scenario that was written passes
  RC_CHECK(c.size() == 8 && c[1].id == "G2" && c[1].status == CoverageStatus::uncovered);
  RC_CHECK(text.find("G2  unacceptable  keeps driving when the link is lost           NOT COVERED") != std::string::npos);
  RC_CHECK(text.find("not covered: G2, G3, G5, G6, G7") != std::string::npos);
  std::cout << "  without S4 the suite is green, and G2, the unacceptable one, is in the list of what has no test\n\n";
}
