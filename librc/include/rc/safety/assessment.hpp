// rc/safety/assessment.hpp
//
// A checkable risk assessment, from lesson 19-14, graduated: the phase's capstone.
//
// An Entry is a hazard (as scored in 19-01), the demands per year and Mitigations, each naming a mechanism, a group (software, hardware, procedure), a pfd and evidence (a function that exercises the mechanism). assess gives credit only to a mitigation whose mechanism is known and whose evidence passes; counts only the best of each group, with beta as the common-cause share between groups (combined_pfd); works out the residual class from demands times pfd; and reports findings: unknown mechanism, evidence failed, no mitigation (or none that survives checking), a catastrophe needing two independent groups, a residual of high or worse, and a catastrophe not accepted by name. Defensible means no findings.
//
// The eight ground vehicle hazards, with real evidence run against the rest of rc::safety, were defensible: G2 went from unacceptable to medium, none was above medium after, and every severity 4 hazard was accepted by name. Without G2's hardware brake it reported two findings and a residual of high; a brake applied by power failed its evidence in 3 hazards; and six changes (an unknown mechanism, a missing acceptor, an unaddressed hazard, a pfd that was not a number, a repeated id, a second software mitigation) were each caught.

#ifndef RC_SAFETY_ASSESSMENT_HPP
#define RC_SAFETY_ASSESSMENT_HPP

#include <algorithm>
#include <cmath>
#include <functional>
#include <map>
#include <rc/safety/hazards.hpp>
#include <rc/safety/risk_math.hpp>
#include <set>
#include <string>
#include <vector>

namespace rc {
namespace safety {

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
  std::map<std::string, double> best;
  for (const Mitigation& m : credited) {
    const double p = rc::safety::clamp_probability(m.pfd);
    auto it = best.find(m.group);
    if (it == best.end() || p < it->second) best[m.group] = p;
  }
  if (best.empty()) return 1.0;
  const double b = std::max(0.0, std::min(1.0, beta));
  double product = 1.0;
  double lowest = 1.0;
  for (const auto& kv : best) {
    product *= (best.size() == 1 ? 1.0 : 1.0 - b) * kv.second;
    lowest = std::min(lowest, kv.second);
  }
  if (best.size() == 1) return product;
  return std::min(1.0, product + b * lowest);
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
  Report report;
  rc::safety::HazardLog log;
  for (const Entry& e : entries) {
    const rc::safety::AddResult added = log.add(e.hazard);
    if (added != rc::safety::AddResult::ok) report.findings.push_back({e.hazard.id, "hazard not accepted into the log"});
    std::vector<Mitigation> credited;
    std::set<std::string> groups;
    for (const Mitigation& m : e.mitigations) {
      bool ok = true;
      if (known.count(m.mechanism) == 0) {
        report.findings.push_back({e.hazard.id, "unknown mechanism: " + m.mechanism});
        ok = false;
      }
      bool passed = false;
      if (m.evidence) {
        try {
          passed = m.evidence();
        } catch (...) {
          passed = false;
        }
      }
      if (!passed) {
        report.findings.push_back({e.hazard.id, "evidence failed: " + m.id});
        ok = false;
      }
      if (ok) {
        credited.push_back(m);
        groups.insert(m.group);
      }
    }
    const rc::safety::RiskClass initial = rc::safety::risk_class(e.hazard.severity, e.hazard.likelihood);
    if (initial != rc::safety::RiskClass::low && credited.empty())
      report.findings.push_back({e.hazard.id, e.mitigations.empty() ? "no mitigation" : "no mitigation survives checking"});
    if (e.hazard.severity == 4 && groups.size() < 2) report.findings.push_back({e.hazard.id, "a catastrophe needs two independent groups"});
    Row row;
    row.id = e.hazard.id;
    row.initial = initial;
    row.credited = static_cast<int>(credited.size());
    row.pfd = combined_pfd(credited, beta);
    row.frequency = rc::safety::hazard_frequency(e.demands_per_year, row.pfd);
    row.likelihood = rc::safety::likelihood_from_frequency(row.frequency);
    row.residual = rc::safety::risk_class(e.hazard.severity, row.likelihood);
    if (row.residual == rc::safety::RiskClass::high || row.residual == rc::safety::RiskClass::unacceptable) report.findings.push_back({e.hazard.id, "residual risk is high or worse"});
    if (e.hazard.severity == 4 && e.accepted_by.empty()) report.findings.push_back({e.hazard.id, "residual risk of a catastrophe is not accepted by name"});
    report.rows.push_back(row);
  }
  report.defensible = report.findings.empty();
  return report;
}

}  // namespace safety
}  // namespace rc

#endif  // RC_SAFETY_ASSESSMENT_HPP
