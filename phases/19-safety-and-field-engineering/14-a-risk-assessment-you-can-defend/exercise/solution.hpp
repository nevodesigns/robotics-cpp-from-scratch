#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <algorithm>
#include <cmath>
#include <functional>
#include <map>
#include <rc/safety/hazards.hpp>
#include <rc/safety/risk_math.hpp>
#include <set>
#include <string>
#include <vector>

// One thing done about a hazard. It names the mechanism that does it (something that exists), the group the mechanism belongs to (software, hardware, procedure: things in one group can fail for the same reason), the chance that it is dead
// when it is needed, and the evidence: a function that exercises the mechanism and says whether it did its job.
struct Mitigation {
  std::string id;
  std::string mechanism;
  std::string group;
  double pfd = 1.0;
  std::function<bool()> evidence;
};

struct Entry {
  rc::safety::Hazard hazard;                 // the hazard as scored in lesson 19-01, before any mitigation
  double demands_per_year = 0.0;             // how often the situation arises
  std::vector<Mitigation> mitigations;
  std::string accepted_by;                   // who has accepted what is left, by name
};

struct Row {
  std::string id;
  rc::safety::RiskClass initial = rc::safety::RiskClass::unacceptable;
  rc::safety::RiskClass residual = rc::safety::RiskClass::unacceptable;
  double pfd = 1.0;
  double frequency = 0.0;                    // per year, after the mitigations
  int likelihood = 5;
  int credited = 0;                          // mitigations that were given credit
};

struct Finding {
  std::string hazard;
  std::string problem;
};

struct Report {
  std::vector<Row> rows;
  std::vector<Finding> findings;
  bool defensible = false;
};

// The chance that the protection fails when needed, from the mitigations that have been given credit. Within one group nothing is gained by having two (they can fail for the same reason), so only the best of each group counts; across groups,
// they are redundant with a common-cause share beta: the product of (1 - beta) pfd over the groups, plus beta times the best pfd. With one group it is that group's pfd; with none, 1. A pfd that is not a number is 1.
inline double combined_pfd(const std::vector<Mitigation>& credited, double beta) {
  // TODO: Keep, for each group, the lowest clamped pfd (rc::safety::clamp_probability). None at all: 1. One group: that pfd.
  // Several groups: product over the groups of (1 - b) * pfd, plus b * the lowest pfd (b is beta kept within 0..1), at most 1
  (void)credited;
  (void)beta;
  return 1.0;
}

// The assessment. Each entry's hazard goes into a hazard log (a malformed or repeated one is a finding), and then for each entry, in order:
//  - a mitigation whose mechanism is not in `known` is a finding ("unknown mechanism: ..."), and gets no credit;
//  - a mitigation whose evidence returns false, throws or is missing is a finding ("evidence failed: <id>"), and gets no credit;
//  - a hazard of class medium or worse with no mitigation is a finding ("no mitigation"), and with mitigations but none credited, "no mitigation survives checking";
//  - a hazard of severity 4 needs credited mitigations from at least two groups ("a catastrophe needs two independent groups");
//  - the residual risk is the severity with the likelihood from demands times combined_pfd of the credited mitigations: a residual class of high or worse is a finding ("residual risk is high or worse");
//  - a severity 4 hazard, whose residual is never below medium, needs a named person who accepted it: an empty accepted_by is a finding ("residual risk of a catastrophe is not accepted by name").
// Defensible means no findings at all.
inline Report assess(const std::vector<Entry>& entries, const std::set<std::string>& known, double beta) {
  // TODO: Follow the comment above: for each entry, add its hazard to a rc::safety::HazardLog (not ok is a finding "hazard not accepted into the log"); check each mitigation's mechanism ("unknown mechanism: <name>") and evidence (run it in try/catch, a missing function fails; "evidence failed: <id>"); only mitigations with both get credit.
  // Then "no mitigation" / "no mitigation survives checking" for a hazard that is not low with none credited; "a catastrophe needs two independent groups" for severity 4 with fewer than two groups credited.
  // Fill a Row: initial = risk_class(severity, likelihood), pfd = combined_pfd(credited, beta), frequency = hazard_frequency(demands, pfd), likelihood = likelihood_from_frequency(frequency), residual = risk_class(severity, that likelihood).
  // "residual risk is high or worse" for a high or unacceptable residual, and "residual risk of a catastrophe is not accepted by name" for severity 4 with an empty accepted_by. defensible is true if there are no findings
  (void)entries;
  (void)known;
  (void)beta;
  return Report();
}

#endif  // LESSON_SOLUTION_HPP
