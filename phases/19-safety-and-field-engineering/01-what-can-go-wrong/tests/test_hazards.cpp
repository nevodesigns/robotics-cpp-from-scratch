#include <rc/test/rc_test.hpp>

#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

#include "solution.hpp"

namespace {

Hazard make(const char* id, const char* what, int s, int l, const char* owner, std::vector<std::string> mitigations) {
  Hazard h;
  h.id = id;
  h.description = what;
  h.severity = s;
  h.likelihood = l;
  h.owner = owner;
  h.mitigations = std::move(mitigations);
  return h;
}

std::string joined(const std::vector<std::string>& v) {
  std::string s;
  for (const std::string& x : v) s += (s.empty() ? "" : ", ") + x;
  return s.empty() ? "none" : s;
}

}  // namespace

RC_TEST("the score is severity times likelihood, and out of range gives nothing to rank by") {
  RC_CHECK(risk_score(4, 5) == 20);
  RC_CHECK(risk_score(1, 1) == 1);
  RC_CHECK(risk_score(3, 3) == 9);
  RC_CHECK(risk_score(0, 3) == 0);
  RC_CHECK(risk_score(5, 3) == 0);
  RC_CHECK(risk_score(2, 0) == 0);
  RC_CHECK(risk_score(2, 6) == 0);
  RC_CHECK(risk_score(-1, -1) == 0);
}

RC_TEST("the classes change at 5, 10 and 15, and a catastrophe is never called low however unlikely") {
  std::cout << "  severity down, likelihood across: class (score)\n               improbable   remote      occasional   probable    frequent\n";
  for (int s = 4; s >= 1; --s) {
    std::cout << "  severity " << s << "   ";
    for (int l = 1; l <= 5; ++l) std::cout << std::left << std::setw(8) << describe(risk_class(s, l)) << "(" << std::right << std::setw(2) << risk_score(s, l) << ")  " << std::left;
    std::cout << std::right << "\n";
  }
  std::cout << "\n";
  RC_CHECK(risk_class(2, 2) == RiskClass::low);                // 4
  RC_CHECK(risk_class(1, 5) == RiskClass::medium);             // 5
  RC_CHECK(risk_class(3, 3) == RiskClass::medium);             // 9
  RC_CHECK(risk_class(2, 5) == RiskClass::high);               // 10
  RC_CHECK(risk_class(3, 4) == RiskClass::high);               // 12
  RC_CHECK(risk_class(3, 5) == RiskClass::unacceptable);       // 15
  RC_CHECK(risk_class(4, 5) == RiskClass::unacceptable);       // 20
  RC_CHECK(risk_class(4, 1) == RiskClass::medium);             // 4 by the product, which would be low
  RC_CHECK(risk_class(4, 2) == RiskClass::medium);             // 8
  RC_CHECK(risk_class(3, 1) == RiskClass::low);                // the floor is for catastrophe only
}

RC_TEST("a hazard that is not well formed is refused for the first thing wrong with it, and is not kept") {
  HazardLog log;
  RC_CHECK(log.add(make("H1", "collision with a person", 4, 3, "safety lead", {})) == AddResult::ok);
  RC_CHECK(log.add(make("", "no name", 2, 2, "x", {})) == AddResult::no_id);
  RC_CHECK(log.add(make("H1", "the same id again", 2, 2, "x", {})) == AddResult::duplicate);
  RC_CHECK(log.add(make("H2", "severity 5", 5, 2, "x", {})) == AddResult::bad_severity);
  RC_CHECK(log.add(make("H3", "likelihood 0", 2, 0, "x", {})) == AddResult::bad_likelihood);
  RC_CHECK(log.add(make("H4", "nobody answers for it", 2, 2, "", {})) == AddResult::no_owner);
  RC_CHECK(log.add(make("", "everything wrong", 9, 9, "", {})) == AddResult::no_id);            // the first failure is the answer
  RC_CHECK(log.add(make("H5", "both bad", 0, 0, "x", {})) == AddResult::bad_severity);
  RC_CHECK(log.size() == 1);
  std::cout << "  six refused entries and one accepted: the log holds " << log.size() << "\n\n";
}

RC_TEST("the log ranks the worst first, and breaks ties by severity and then by name so that the order is always the same") {
  HazardLog log;
  log.add(make("H-low", "a scuffed bumper", 1, 2, "ops", {"inspection"}));                        // 2
  log.add(make("H-b", "equal score, lower severity", 2, 5, "ops", {"x"}));            // 10
  log.add(make("H-a", "equal score, higher severity", 4, 3, "ops", {"x"}));                       // 12
  log.add(make("H-c", "ties with H-a on score and severity", 3, 4, "ops", {"x"}));                // 12
  log.add(make("H-top", "worst", 4, 5, "ops", {"x"}));                                            // 20
  const std::vector<Hazard> r = log.ranked();
  std::cout << "  ranked:";
  for (const Hazard& h : r) std::cout << " " << h.id << "(" << risk_score(h.severity, h.likelihood) << ")";
  std::cout << "\n\n";
  RC_CHECK(r.size() == 5);
  if (r.size() != 5) return;
  RC_CHECK(r[0].id == "H-top");
  RC_CHECK(r[1].id == "H-a");           // 12, severity 4
  RC_CHECK(r[2].id == "H-c");           // 12, severity 3
  RC_CHECK(r[3].id == "H-b");
  RC_CHECK(r[4].id == "H-low");
}

RC_TEST("what has nothing done about it is listed worst first, and a low risk with no mitigation is not an omission but a catastrophe with none is") {
  HazardLog log;
  log.add(make("U1", "collision with a person", 4, 3, "safety lead", {}));                         // high, nothing done
  log.add(make("U2", "rollover on a slope", 3, 3, "mech", {"tilt limit"}));                        // medium, handled
  log.add(make("U3", "a scuffed bumper", 1, 2, "ops", {}));                                        // low, nothing done: fine
  log.add(make("U4", "runaway on loss of link", 4, 4, "controls", {}));                            // unacceptable, nothing done
  log.add(make("U5", "a very unlikely fire", 4, 1, "mech", {}));                                   // catastrophic: floor of medium, nothing done
  log.add(make("U6", "an unlikely minor pinch", 2, 1, "mech", {}));                                // low: fine
  const std::vector<std::string> open = log.unaddressed();
  std::cout << "  six hazards, four with no mitigation: unaddressed, worst first: " << joined(open) << "\n\n";
  RC_CHECK(open.size() == 3);
  if (open.size() != 3) return;
  RC_CHECK(open[0] == "U4");
  RC_CHECK(open[1] == "U1");
  RC_CHECK(open[2] == "U5");
  HazardLog empty;
  RC_CHECK(empty.unaddressed().empty());
  RC_CHECK(empty.ranked().empty());
}

RC_TEST("an autonomous ground vehicle's first hazards, scored, are the ones the rest of this phase has to answer") {
  HazardLog log;
  const struct { const char* id; const char* what; int s; int l; } rows[] = {
      {"G1", "runs into a person", 4, 3},
      {"G2", "keeps driving when the link is lost", 4, 4},
      {"G3", "rolls over on a slope", 3, 3},
      {"G4", "cannot stop in the distance its sensor sees", 4, 3},
      {"G5", "battery fails in the field", 2, 4},
      {"G6", "motor overheats and burns", 3, 2},
      {"G7", "leaves the area it is allowed in", 3, 3},
      {"G8", "a sensor fails and nothing notices", 3, 4},
  };
  for (const auto& r : rows) log.add(make(r.id, r.what, r.s, r.l, "safety lead", {}));
  const std::vector<Hazard> r = log.ranked();
  int high = 0;
  int medium = 0;
  int unacceptable = 0;
  std::cout << "  hazard   score   class          what\n";
  for (const Hazard& h : r) {
    const RiskClass c = risk_class(h.severity, h.likelihood);
    if (c == RiskClass::high) ++high;
    if (c == RiskClass::medium) ++medium;
    if (c == RiskClass::unacceptable) ++unacceptable;
    std::cout << "  " << std::left << std::setw(7) << h.id << "  " << std::right << std::setw(5) << risk_score(h.severity, h.likelihood) << "   " << std::left << std::setw(13) << describe(c) << "  " << h.description
              << std::right << "\n";
  }
  std::cout << "\n  " << unacceptable << " unacceptable, " << high << " high, " << medium << " medium, none addressed yet: " << log.unaddressed().size() << " open\n\n";
  RC_CHECK(r.size() == 8);
  RC_CHECK(unacceptable == 1);
  RC_CHECK(high == 3);
  RC_CHECK(medium == 4);
  RC_CHECK(log.unaddressed().size() == 8);
  RC_CHECK(r[0].id == "G2");
}
