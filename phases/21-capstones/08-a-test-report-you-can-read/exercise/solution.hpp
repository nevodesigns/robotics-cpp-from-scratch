#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <functional>
#include <iomanip>
#include <rc/capstone/delivery_rover.hpp>
#include <rc/safety/hazards.hpp>
#include <rc/safety/supervisor.hpp>
#include <sstream>
#include <string>
#include <vector>

// The building of lesson 21-02: two rooms of 5 by 6 m with a door of 1.5 m between them.
inline rc::capstone::Building report_building() {
  std::vector<std::string> rows(24, std::string(40, '.'));
  for (int r = 0; r < 24; ++r) {
    rows[static_cast<std::size_t>(r)][0] = '#';
    rows[static_cast<std::size_t>(r)][39] = '#';
  }
  rows[0] = std::string(40, '#');
  rows[23] = std::string(40, '#');
  for (int y = 0; y < 24; ++y)
    if (y < 9 || y > 14) rows[static_cast<std::size_t>(23 - y)][20] = '#';
  return rc::capstone::parse_building(rows);
}

// One test of the rover: what is put into the building, what the rover is configured to do, which hazards of the log (lesson 19-01) the test is for, and the rule that says it passed.
struct SuiteScenario {
  std::string id;                                      // S1, S2 ...
  std::string title;
  std::vector<std::string> hazards;                    // the ids of the hazards it is evidence for
  rc::capstone::DeliveryConfig config;
  std::vector<rc::capstone::Person> people;
  double limit = 60.0;                                 // seconds
  std::function<bool(const rc::capstone::DeliveryResult&)> passes;
};

struct SuiteRow {
  std::string id;
  bool passed = false;
  rc::capstone::DeliveryResult result;
};

// Run every scenario against the building, from the cell (3, 3) to the cell (36, 20). A scenario with no rule fails. The rows are in the order of the scenarios.
inline std::vector<SuiteRow> run_suite(const std::vector<SuiteScenario>& scenarios) {
  const rc::capstone::Building b = report_building();
  std::vector<SuiteRow> rows;
  for (const SuiteScenario& s : scenarios) {
    SuiteRow row;
    row.id = s.id;
    row.result = rc::capstone::run_delivery(b, s.people, {3, 3}, {36, 20}, s.config, s.limit);
    row.passed = s.passes ? s.passes(row.result) : false;
    rows.push_back(row);
  }
  return rows;
}

enum class CoverageStatus { covered, failing, uncovered };

struct HazardLine {
  std::string id;
  std::string description;
  rc::safety::RiskClass risk = rc::safety::RiskClass::low;
  CoverageStatus status = CoverageStatus::uncovered;
  std::vector<std::string> scenarios;                  // the scenarios that name it
};

// For each hazard whose class is medium or worse (rc::safety::risk_class on its severity and likelihood), in the order of the list: the scenarios that name it, and its status: `uncovered` if there are none, `failing` if any of them failed,
// `covered` if there is at least one and all passed. Hazards of class low are left out.
inline std::vector<HazardLine> hazard_coverage(const std::vector<rc::safety::Hazard>& hazards, const std::vector<SuiteScenario>& scenarios, const std::vector<SuiteRow>& rows) {
  // TODO: as the comment above: skip hazards of class low; for each other, the ids of the scenarios that name it, and a status uncovered (none), failing (any of them failed) or covered
  (void)hazards;
  (void)scenarios;
  (void)rows;
  return std::vector<HazardLine>();
}

// The report as text, one thing to a line, in this form (columns padded with spaces to the widths shown, the numbers with one decimal where a time and two where a gap):
//   Scenarios
//   S1  PASS  <title padded to 34>  reached 12.3 s, 0 bumps, closest 0.18 m          (or "not reached, 0 bumps, stopped: link" with the causes_text of the causes, or "not reached, 0 bumps" if no causes)
//   ...
//   (a blank line)
//   Hazards of class medium or worse
//   G2  unacceptable  <description padded to 44>  covered by S4          (or "FAILING in S2, S3" or "NOT COVERED")
//   ...
//   (a blank line)
//   Summary: <passed> of <n> scenarios passed; <covered> of <m> hazards covered by passing scenarios[; not covered: G3, G5][; failing: G1]
// The risk class is written by rc::safety::describe. Each line ends with a newline.
inline std::string render_report(const std::vector<SuiteScenario>& scenarios, const std::vector<SuiteRow>& rows, const std::vector<HazardLine>& coverage) {
  // TODO: follow the form in the comment above exactly: std::ostringstream, std::left and std::setw for the padded columns, std::fixed and std::setprecision for the numbers
  (void)scenarios;
  (void)rows;
  (void)coverage;
  return std::string();
}

// Is the whole suite green: every scenario passed, and no hazard of class medium or worse has a failing scenario (an uncovered hazard is a gap and is reported, but it is not a failure of what was tested).
inline bool suite_green(const std::vector<SuiteRow>& rows, const std::vector<HazardLine>& coverage) {
  // TODO: false if any row failed or any hazard line is failing; an uncovered one does not make it false
  (void)rows;
  (void)coverage;
  return false;
}

#endif  // LESSON_SOLUTION_HPP
