#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <algorithm>
#include <cstddef>
#include <string>
#include <vector>

// How bad it is if it happens: 1 negligible, 2 minor injury or damage, 3 serious injury or major damage, 4 death or loss of the machine.
// How likely it is over the life of the machine: 1 improbable, 2 remote, 3 occasional, 4 probable, 5 frequent.
enum class RiskClass { low, medium, high, unacceptable };

inline const char* describe(RiskClass c) {
  switch (c) {
    case RiskClass::low: return "low";
    case RiskClass::medium: return "medium";
    case RiskClass::high: return "high";
    case RiskClass::unacceptable: return "unacceptable";
  }
  return "?";
}

// Severity times likelihood, 1 to 20. Zero for either out of range.
inline int risk_score(int severity, int likelihood) {
  if (severity < 1 || severity > 4 || likelihood < 1 || likelihood > 5) return 0;
  return severity * likelihood;
}

// The class of a risk: a score of up to 4 is low, 5 to 9 medium, 10 to 14 high and 15 or more unacceptable.
// But a product hides what a catastrophe is: severity 4 with likelihood 1 scores 4, which is low. A catastrophic outcome is never classed below medium,
// however unlikely it is thought to be, because the likelihood is a guess and the outcome is not.
inline RiskClass risk_class(int severity, int likelihood) {
  const int s = risk_score(severity, likelihood);
  RiskClass c = RiskClass::low;
  if (s >= 15) c = RiskClass::unacceptable;
  else if (s >= 10) c = RiskClass::high;
  else if (s >= 5) c = RiskClass::medium;
  if (severity == 4 && c == RiskClass::low) c = RiskClass::medium;
  return c;
}

struct Hazard {
  std::string id;
  std::string description;
  int severity = 0;
  int likelihood = 0;
  std::string owner;                         // who answers for it
  std::vector<std::string> mitigations;      // what is done about it
};

enum class AddResult { ok, no_id, duplicate, bad_severity, bad_likelihood, no_owner };

// The hazards of a machine.
class HazardLog {
 public:
  // Accept a hazard only if it is well formed: an id that is not empty and not already used, a severity of 1 to 4, a likelihood of 1 to 5 and an owner.
  // The checks are made in that order and the first failure is the answer. A hazard that is refused is not added.
  AddResult add(const Hazard& h) {
    if (h.id.empty()) return AddResult::no_id;
    for (const Hazard& x : hazards_)
      if (x.id == h.id) return AddResult::duplicate;
    if (h.severity < 1 || h.severity > 4) return AddResult::bad_severity;
    if (h.likelihood < 1 || h.likelihood > 5) return AddResult::bad_likelihood;
    if (h.owner.empty()) return AddResult::no_owner;
    hazards_.push_back(h);
    return AddResult::ok;
  }

  std::size_t size() const { return hazards_.size(); }

  // Worst first: by score, then by severity, then by id.
  std::vector<Hazard> ranked() const {
    std::vector<Hazard> out = hazards_;
    std::sort(out.begin(), out.end(), [](const Hazard& a, const Hazard& b) {
      const int sa = risk_score(a.severity, a.likelihood);
      const int sb = risk_score(b.severity, b.likelihood);
      if (sa != sb) return sa > sb;
      if (a.severity != b.severity) return a.severity > b.severity;
      return a.id < b.id;
    });
    return out;
  }

  // The ids, worst first, of hazards of class medium or worse that have no mitigation.
  std::vector<std::string> unaddressed() const {
    std::vector<std::string> ids;
    for (const Hazard& h : ranked())
      if (risk_class(h.severity, h.likelihood) != RiskClass::low && h.mitigations.empty()) ids.push_back(h.id);
    return ids;
  }

 private:
  std::vector<Hazard> hazards_;
};

#endif  // LESSON_SOLUTION_HPP
