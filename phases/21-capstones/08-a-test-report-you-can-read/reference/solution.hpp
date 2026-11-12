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
  std::vector<HazardLine> out;
  for (const rc::safety::Hazard& h : hazards) {
    const rc::safety::RiskClass c = rc::safety::risk_class(h.severity, h.likelihood);
    if (c == rc::safety::RiskClass::low) continue;
    HazardLine line;
    line.id = h.id;
    line.description = h.description;
    line.risk = c;
    bool any_failed = false;
    for (std::size_t i = 0; i < scenarios.size() && i < rows.size(); ++i) {
      bool names = false;
      for (const std::string& id : scenarios[i].hazards)
        if (id == h.id) names = true;
      if (!names) continue;
      line.scenarios.push_back(scenarios[i].id);
      if (!rows[i].passed) any_failed = true;
    }
    line.status = line.scenarios.empty() ? CoverageStatus::uncovered : (any_failed ? CoverageStatus::failing : CoverageStatus::covered);
    out.push_back(line);
  }
  return out;
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
  std::ostringstream out;
  out << "Scenarios\n";
  int passed = 0;
  for (std::size_t i = 0; i < scenarios.size() && i < rows.size(); ++i) {
    const rc::capstone::DeliveryResult& r = rows[i].result;
    if (rows[i].passed) ++passed;
    out << scenarios[i].id << "  " << (rows[i].passed ? "PASS" : "FAIL") << "  " << std::left << std::setw(34) << scenarios[i].title << "  ";
    if (r.reached) {
      out << "reached " << std::fixed << std::setprecision(1) << r.time << " s, " << r.bumps << " bumps, closest " << std::setprecision(2) << r.min_gap << " m";
    } else {
      out << "not reached, " << r.bumps << " bumps";
      if (r.causes != 0) out << ", stopped: " << rc::safety::causes_text(r.causes);
    }
    out << "\n";
  }
  out << "\nHazards of class medium or worse\n";
  int covered = 0;
  std::string none;
  std::string failing;
  for (const HazardLine& h : coverage) {
    out << h.id << "  " << std::left << std::setw(12) << rc::safety::describe(h.risk) << "  " << std::setw(44) << h.description << "  ";
    std::string list;
    for (const std::string& s : h.scenarios) list += (list.empty() ? "" : ", ") + s;
    if (h.status == CoverageStatus::covered) {
      out << "covered by " << list;
      ++covered;
    } else if (h.status == CoverageStatus::failing) {
      out << "FAILING in " << list;
      failing += (failing.empty() ? "" : ", ") + h.id;
    } else {
      out << "NOT COVERED";
      none += (none.empty() ? "" : ", ") + h.id;
    }
    out << "\n";
  }
  out << "\nSummary: " << passed << " of " << scenarios.size() << " scenarios passed; " << covered << " of " << coverage.size() << " hazards covered by passing scenarios";
  if (!none.empty()) out << "; not covered: " << none;
  if (!failing.empty()) out << "; failing: " << failing;
  out << "\n";
  return out.str();
}

// Is the whole suite green: every scenario passed, and no hazard of class medium or worse has a failing scenario (an uncovered hazard is a gap and is reported, but it is not a failure of what was tested).
inline bool suite_green(const std::vector<SuiteRow>& rows, const std::vector<HazardLine>& coverage) {
  for (const SuiteRow& r : rows)
    if (!r.passed) return false;
  for (const HazardLine& h : coverage)
    if (h.status == CoverageStatus::failing) return false;
  return true;
}

#endif  // LESSON_SOLUTION_HPP
