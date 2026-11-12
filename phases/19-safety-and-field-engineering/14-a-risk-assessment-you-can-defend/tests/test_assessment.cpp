#include <rc/test/rc_test.hpp>
#include <rc/safety/cross_check.hpp>
#include <rc/safety/failsafe.hpp>
#include <rc/safety/geofence.hpp>
#include <rc/safety/hazards.hpp>
#include <rc/safety/limiter.hpp>
#include <rc/safety/power.hpp>
#include <rc/safety/risk_math.hpp>
#include <rc/safety/selftest.hpp>
#include <rc/safety/stop_path.hpp>
#include <rc/safety/stopping.hpp>
#include <rc/safety/supervisor.hpp>
#include <rc/safety/tilt.hpp>

#include <cmath>
#include <iomanip>
#include <iostream>
#include <limits>

#include "solution.hpp"

namespace {

namespace rs = rc::safety;

const double kNaN = std::numeric_limits<double>::quiet_NaN();

// The mechanisms of this phase, by name.
std::set<std::string> known_mechanisms() {
  return {"rc::safety::StopPath", "rc::safety::brake_engaged", "rc::safety::limit_speed", "rc::safety::max_speed", "rc::safety::fence_state", "rc::safety::CrossCheck", "rc::safety::PowerMonitor",
          "rc::safety::TiltMonitor", "rc::safety::max_turn_speed", "rc::safety::Supervisor", "rc::safety::SelfTest", "rc::safety::BlackBox"};
}

// The evidence: each exercises the real code and says whether it does its job.
bool ev_stop_path() {
  rs::StopPath p(0.3);
  p.estop_line(true);
  p.heartbeat(0.0);
  if (!p.reset(0.0)) return false;
  return p.output(1.0, 0.1) == 1.0 && p.output(1.0, 1.0) == 0.0 && p.output(1.0, 1.1) == 0.0;
}
bool ev_spring_brake(rs::BrakeDesign d) { return rs::brake_engaged(d, 0.0, true) && rs::brake_engaged(d, 24.0, false); }
bool ev_limiter() {
  const rs::LimiterConfig c;
  rs::Reading near;
  near.range = 1.0;
  rs::Reading none;
  none.range = kNaN;
  rs::Reading far;
  far.range = 8.0;
  return rs::limit_speed(2.0, near, far, c) < 1.0 && rs::limit_speed(2.0, none, far, c) == 0.0 && rs::limit_speed(2.0, far, far, c) == 2.0;
}
bool ev_stopping() {
  const double v = rs::max_speed(3.0, 2.0, 0.5, 0.3);
  return rs::stopping_distance(v, 2.0, 0.5) <= 2.7 + 1e-9 && rs::stopping_distance(1.0, rs::effective_decel(3.0, 0.3, 17.0 * 3.14159265358979 / 180.0), 0.3) > 1e9;
}
bool ev_fence() {
  const std::vector<rs::Point> sq = {{0, 0}, {20, 0}, {20, 20}, {0, 20}};
  return rs::fence_state(sq, {10, 10}, 2.0, 1.0) == rs::FenceState::inside && rs::fence_state(sq, {1, 10}, 2.0, 1.0) == rs::FenceState::outside && rs::fence_state(sq, {kNaN, 1}, 2.0, 1.0) == rs::FenceState::unknown;
}
bool ev_cross_check() {
  rs::CrossConfig c;
  c.tolerance = 0.1;
  c.debounce = 3;
  rs::CrossCheck ck(c);
  ck.update(1.0, 1.0);
  ck.update(1.0, 2.0);
  ck.update(1.0, 2.0);
  return ck.update(1.0, 2.0).state == rs::CheckState::faulted && ck.faulted();
}
bool ev_power() {
  rs::PowerMonitor m{rs::PowerConfig()};
  return m.update(24.0, 0.0, 25.0).state == rs::PowerState::ok && m.update(21.0, 0.0, 25.0).state == rs::PowerState::derate && m.update(24.0, 0.0, 85.0).state == rs::PowerState::stop;
}
bool ev_tilt() {
  rs::TiltMonitor t(0.26, 0.44, 3);
  t.update(0.5);
  t.update(0.5);
  return t.update(0.5) == rs::TiltState::stop && rs::max_turn_speed(1.0, 0.5, 0.5, 0.5) < rs::max_turn_speed(1.0, 0.5, 0.3, 0.5);
}
bool ev_supervisor() {
  rs::Supervisor s{rs::SupervisorConfig()};
  const rs::Decision d = s.step(rs::Inputs());
  return d.mode == rs::Mode::stopped && d.speed_cap == 0.0 && !s.reset(rs::Inputs());
}

rs::Hazard hz(const char* id, const char* what, int s, int l) {
  rs::Hazard h;
  h.id = id;
  h.description = what;
  h.severity = s;
  h.likelihood = l;
  h.owner = "safety lead";
  return h;
}

Mitigation mit(const char* id, const char* mech, const char* group, double pfd, std::function<bool()> ev) {
  Mitigation m;
  m.id = id;
  m.mechanism = mech;
  m.group = group;
  m.pfd = pfd;
  m.evidence = std::move(ev);
  return m;
}

Entry entry(rs::Hazard h, double demands, std::vector<Mitigation> ms, const char* accepted = "") {
  Entry e;
  e.hazard = std::move(h);
  e.demands_per_year = demands;
  e.mitigations = std::move(ms);
  e.accepted_by = accepted;
  return e;
}

const double kSoftware = 3.641e-3;     // a software function with a dangerous failure rate of 1e-5 per hour, self-tested monthly (lesson 19-13)
const double kBrake = 1e-4;            // a spring applied brake, from a data sheet: an input, and the one to question

std::vector<Entry> ground_vehicle(rs::BrakeDesign brake = rs::BrakeDesign::spring_applied) {
  auto spring = [brake](const char* id) { return mit(id, "rc::safety::brake_engaged", "hardware", kBrake, [brake] { return ev_spring_brake(brake); }); };
  std::vector<Entry> v;
  v.push_back(entry(hz("G1", "runs into a person", 4, 3), 10.0, {mit("G1-limiter", "rc::safety::limit_speed", "software", kSoftware, ev_limiter), spring("G1-brake")}, "safety lead"));
  v.push_back(entry(hz("G2", "keeps driving when the link is lost", 4, 4), 20.0, {mit("G2-stop-path", "rc::safety::StopPath", "software", kSoftware, ev_stop_path), spring("G2-brake")}, "safety lead"));
  v.push_back(entry(hz("G3", "rolls over on a slope", 3, 3), 5.0, {mit("G3-tilt", "rc::safety::TiltMonitor", "software", kSoftware, ev_tilt), mit("G3-turn", "rc::safety::max_turn_speed", "software", kSoftware, ev_tilt)}));
  v.push_back(entry(hz("G4", "cannot stop in the distance its sensor sees", 4, 3), 30.0, {mit("G4-stopping", "rc::safety::max_speed", "software", kSoftware, ev_stopping), spring("G4-brake")}, "safety lead"));
  v.push_back(entry(hz("G5", "battery fails in the field", 2, 4), 12.0, {mit("G5-power", "rc::safety::PowerMonitor", "software", kSoftware, ev_power)}));
  v.push_back(entry(hz("G6", "motor overheats and burns", 3, 2), 6.0, {mit("G6-power", "rc::safety::PowerMonitor", "software", kSoftware, ev_power)}));
  v.push_back(entry(hz("G7", "leaves the area it is allowed in", 3, 3), 10.0, {mit("G7-fence", "rc::safety::fence_state", "software", kSoftware, ev_fence), mit("G7-supervisor", "rc::safety::Supervisor", "software", kSoftware, ev_supervisor)}));
  v.push_back(entry(hz("G8", "a sensor fails and nothing notices", 3, 4), 4.0, {mit("G8-cross-check", "rc::safety::CrossCheck", "software", kSoftware, ev_cross_check)}));
  return v;
}

const char* cls(rs::RiskClass c) { return rs::describe(c); }

}  // namespace

RC_TEST("within a group a second mitigation adds nothing, across groups two are redundant with a common cause, and an unknown pfd counts as certain failure") {
  std::vector<Mitigation> one = {mit("a", "m", "software", 3.6e-3, nullptr)};
  std::vector<Mitigation> twin = {mit("a", "m", "software", 3.6e-3, nullptr), mit("b", "m", "software", 3.6e-3, nullptr)};
  std::vector<Mitigation> two_groups = {mit("a", "m", "software", 3.6e-3, nullptr), mit("b", "m", "hardware", 1e-4, nullptr)};
  const double p1 = combined_pfd(one, 0.1);
  const double p2 = combined_pfd(twin, 0.1);
  const double p3 = combined_pfd(two_groups, 0.1);
  std::cout << "  one software function " << std::scientific << std::setprecision(2) << p1 << "; two in the same group " << p2 << "; software and a hardware brake (beta 0.10) " << p3 << "; beta 0: " << combined_pfd(two_groups, 0.0) << "\n\n";
  RC_CHECK_NEAR(p1, 3.6e-3, 1e-12);
  RC_CHECK_NEAR(p2, p1, 1e-12);
  RC_CHECK(p3 < p1 / 100.0);
  RC_CHECK_NEAR(p3, (0.9 * 3.6e-3) * (0.9 * 1e-4) + 0.1 * 1e-4, 1e-12);
  RC_CHECK(combined_pfd(two_groups, 0.0) < p3);
  RC_CHECK(combined_pfd({}, 0.1) == 1.0);
  RC_CHECK(combined_pfd({mit("a", "m", "software", kNaN, nullptr)}, 0.1) == 1.0);
  RC_CHECK(combined_pfd({mit("a", "m", "software", 1e-3, nullptr), mit("b", "m", "software", 1e-5, nullptr)}, 0.1) == 1e-5);   // the best of the group
}

RC_TEST("the ground vehicle's eight hazards, each with mechanisms that exist and evidence that passes, are defensible, and the residual risk is worked out") {
  const Report r = assess(ground_vehicle(), known_mechanisms(), 0.10);
  std::cout << "  hazard   before         after          pfd        per year    kept\n";
  for (const Row& row : r.rows)
    std::cout << "  " << std::left << std::setw(7) << row.id << "  " << std::setw(13) << cls(row.initial) << "  " << std::setw(13) << cls(row.residual) << "  " << std::right << std::scientific << std::setprecision(2) << row.pfd << "   " << row.frequency
              << "    " << row.credited << " of " << ground_vehicle()[static_cast<std::size_t>(&row - &r.rows[0])].mitigations.size() << "\n";
  std::cout << "  findings: " << r.findings.size() << ", defensible: " << (r.defensible ? "yes" : "no") << "\n\n";
  RC_CHECK(r.rows.size() == 8);
  RC_CHECK(r.findings.empty());
  RC_CHECK(r.defensible);
  if (r.rows.size() != 8) return;
  RC_CHECK(r.rows[1].initial == rs::RiskClass::unacceptable);        // G2
  RC_CHECK(r.rows[1].residual == rs::RiskClass::medium);
  RC_CHECK(r.rows[0].initial == rs::RiskClass::high);
  RC_CHECK(r.rows[0].residual == rs::RiskClass::medium);             // G1: severity 4 is never below medium
  for (const Row& row : r.rows) RC_CHECK(row.residual != rs::RiskClass::high && row.residual != rs::RiskClass::unacceptable);
  RC_CHECK(r.rows[2].credited == 2);                                 // G3: both counted as kept, though in one group
  RC_CHECK_NEAR(r.rows[2].pfd, kSoftware, 1e-12);                    // and worth one
}

RC_TEST("take the hardware brake away from G2, and the assessment says it needs two independent groups and that the residual risk is high") {
  std::vector<Entry> v = ground_vehicle();
  v[1].mitigations.pop_back();
  const Report r = assess(v, known_mechanisms(), 0.10);
  RC_CHECK(r.rows.size() == 8);
  if (r.rows.size() != 8) return;
  std::cout << "  G2 with only the software stop path: defensible " << (r.defensible ? "yes" : "no") << "; findings:\n";
  for (const Finding& f : r.findings) std::cout << "    " << f.hazard << ": " << f.problem << "\n";
  std::cout << "  residual " << cls(r.rows[1].residual) << ", " << std::scientific << std::setprecision(2) << r.rows[1].frequency << " per year\n\n";
  RC_CHECK(!r.defensible);
  RC_CHECK(r.findings.size() == 2);
  if (r.findings.size() != 2) return;
  RC_CHECK(r.findings[0].problem == "a catastrophe needs two independent groups");
  RC_CHECK(r.findings[1].problem == "residual risk is high or worse");
  RC_CHECK(r.rows[1].residual == rs::RiskClass::high);
}

RC_TEST("a brake that is applied by power fails its evidence in every hazard that relied on it, and the credit goes with it") {
  const Report r = assess(ground_vehicle(rs::BrakeDesign::power_applied), known_mechanisms(), 0.10);
  RC_CHECK(r.rows.size() == 8);
  if (r.rows.size() != 8) return;
  int evidence_failed = 0;
  for (const Finding& f : r.findings)
    if (f.problem.rfind("evidence failed", 0) == 0) ++evidence_failed;
  std::cout << "  the power applied brake in G1, G2 and G4: " << evidence_failed << " evidence failures, " << r.findings.size() << " findings in all; G2 residual " << cls(r.rows[1].residual) << "\n";
  for (const Finding& f : r.findings) std::cout << "    " << f.hazard << ": " << f.problem << "\n";
  std::cout << "\n";
  RC_CHECK(!r.defensible);
  RC_CHECK(evidence_failed == 3);
  RC_CHECK(r.rows[1].credited == 1);
  RC_CHECK(r.rows[1].residual == rs::RiskClass::high);
  RC_CHECK(r.findings.size() == 3 * 3);                              // each of G1, G2, G4: the evidence, the two groups, and the residual (or the acceptance)
}

RC_TEST("a mitigation that is only a name, a second one in the same group, a missing acceptor, an unaddressed hazard and a pfd nobody knew are each caught") {
  const std::set<std::string> known = known_mechanisms();
  int caught = 0;
  auto first_problem = [&](const std::vector<Entry>& v, const char* hazard, const std::string& want) {
    const Report r = assess(v, known, 0.10);
    bool found = false;
    for (const Finding& f : r.findings)
      if (f.hazard == hazard && f.problem == want) found = true;
    std::cout << "  " << (found ? "caught " : "MISSED ") << want << " (" << hazard << ")\n";
    if (found && !r.defensible) ++caught;
    return found;
  };
  {
    std::vector<Entry> v = ground_vehicle();
    v[1].mitigations[0].mechanism = "magic watchdog v2";
    RC_CHECK(first_problem(v, "G2", "unknown mechanism: magic watchdog v2"));
  }
  {
    std::vector<Entry> v = ground_vehicle();
    v[0].accepted_by = "";
    RC_CHECK(first_problem(v, "G1", "residual risk of a catastrophe is not accepted by name"));
  }
  {
    std::vector<Entry> v = ground_vehicle();
    v.push_back(entry(hz("G9", "pinch point on the arm", 3, 3), 10.0, {}));
    RC_CHECK(first_problem(v, "G9", "no mitigation"));
  }
  {
    std::vector<Entry> v = ground_vehicle();
    v[4].mitigations[0].pfd = kNaN;
    RC_CHECK(first_problem(v, "G5", "residual risk is high or worse"));
  }
  {
    std::vector<Entry> v = ground_vehicle();
    v.push_back(entry(hz("G1", "the same id again", 2, 2), 1.0, {}));
    RC_CHECK(first_problem(v, "G1", "hazard not accepted into the log"));
  }
  {
    std::vector<Entry> v = ground_vehicle();
    v[6].mitigations.push_back(mit("G7-fence-b", "rc::safety::fence_state", "software", kSoftware, ev_fence));
    const Report r = assess(v, known, 0.10);
    const Report base = assess(ground_vehicle(), known, 0.10);
    RC_CHECK(r.rows.size() == 8 && base.rows.size() == 8);
    if (r.rows.size() != 8 || base.rows.size() != 8) return;
    std::cout << "  a second software mitigation for G7: pfd " << std::scientific << std::setprecision(2) << r.rows[6].pfd << " against " << base.rows[6].pfd << " before: no credit for the second\n";
    RC_CHECK(r.defensible);
    RC_CHECK_NEAR(r.rows[6].pfd, base.rows[6].pfd, 1e-15);
    ++caught;
  }
  std::cout << "  " << caught << " of 6 changes were caught as they should be\n\n";
  RC_CHECK(caught == 6);
}
