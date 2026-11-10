#include <rc/test/rc_test.hpp>

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

#include <rc/test/response.hpp>

#include "solution.hpp"

namespace {

Fault make(const char* name) {
  Fault f;
  f.name = name;
  return f;
}

std::vector<Fault> faults() {
  std::vector<Fault> v;
  Fault a = make("sign flipped");      a.kp = -2.0; a.ki = -3.0;            v.push_back(a);
  Fault b = make("sensor offset");     b.sensor_offset = 0.2;               v.push_back(b);
  Fault c = make("weak actuator");     c.actuator_limit = 0.8;              v.push_back(c);
  Fault d = make("limiter removed");   d.kp = 8.0; d.ki = 12.0; d.limiter = false; v.push_back(d);
  Fault e = make("measurement late");  e.delay_steps = 700;                 v.push_back(e);
  Fault g = make("sensor dropout");    g.dropout_from = 3000; g.dropout_to = 3600; v.push_back(g);
  return v;
}

// Check 1: the response meets the specification of lesson 05-08.
bool response_is_fine(const Run& run) {
  rc::test::Response r;
  r.y = run.y;
  r.dt = run.dt;
  r.target = run.target;
  return rc::test::meets(rc::test::Spec(), rc::test::measure(r));
}

// Check 2: the controller never commanded more than its specification allows.
bool output_within_limit(const Run& run) {
  for (double u : run.u) {
    if (std::fabs(u) > run.declared_limit + 1e-9) return false;
  }
  return true;
}

// Check 3: what the controller was told agrees with a model of the plant driven by what it commanded.
bool sensor_agrees_with_model(const Run& run) {
  double y = 0.0;
  for (std::size_t i = 0; i < run.u.size(); ++i) {
    if (std::fabs(run.seen[i] - y) > 0.05) return false;
    const double delivered = std::max(-run.declared_limit, std::min(run.declared_limit, run.u[i]));
    y += run.dt * (delivered - y) / 0.5;
  }
  return true;
}

// A name that cannot read past the end, so a wrong answer from the code under test is a failed check and not a crash.
const char* name_of(const std::vector<Fault>& fs, std::size_t f) { return f < fs.size() ? fs[f].name : "(no such fault)"; }

std::vector<Check> checks() {
  std::vector<Check> v;
  v.push_back(response_is_fine);
  v.push_back(output_within_limit);
  v.push_back(sensor_agrees_with_model);
  return v;
}

const char* kCheckNames[] = {"response meets spec", "output within limit", "sensor agrees with model"};

std::vector<Run> runs_of(const std::vector<Fault>& fs) {
  std::vector<Run> v;
  for (const Fault& f : fs) v.push_back(simulate_fault(f));
  return v;
}

}  // namespace

RC_TEST("a run with no fault passes every check") {
  const Run clean = simulate_fault(Fault());
  RC_CHECK(response_is_fine(clean));
  RC_CHECK(output_within_limit(clean));
  RC_CHECK(sensor_agrees_with_model(clean));
  RC_CHECK_EQ(false_alarms(checks(), clean), 0);
}

RC_TEST("a check that fails is a fault found, and the table says which check found which fault") {
  const std::vector<Fault> fs = faults();
  const std::vector<std::vector<bool> > caught = detection_matrix(checks(), runs_of(fs));
  RC_CHECK(caught.size() == fs.size());
  if (!(caught.size() == fs.size())) return;
  RC_CHECK(caught[0].size() == 3);
  if (!(caught[0].size() == 3)) return;

  std::cout << "  fault                 ";
  for (const char* n : kCheckNames) std::cout << std::setw(26) << n;
  std::cout << "\n";
  for (std::size_t f = 0; f < fs.size(); ++f) {
    std::cout << "  " << std::left << std::setw(20) << fs[f].name << std::right;
    for (std::size_t c = 0; c < 3; ++c) std::cout << std::setw(26) << (caught[f][c] ? "CAUGHT" : ".");
    std::cout << "\n";
  }
  std::cout << "\n";

  // The behaviour check sees everything that changes how the plant moves.
  RC_CHECK(caught[0][0] && caught[1][0] && caught[2][0] && caught[4][0] && caught[5][0]);
  // The limit check sees only the controller that was let off its limit, and sees nothing else.
  RC_CHECK(caught[3][1]);
  RC_CHECK(!caught[0][1] && !caught[1][1] && !caught[2][1] && !caught[4][1] && !caught[5][1]);
  // The model check sees a sensor that disagrees with the plant: an offset, a late reading and a dropout.
  RC_CHECK(caught[1][2] && caught[4][2] && caught[5][2]);
  RC_CHECK(!caught[0][2] && !caught[2][2] && !caught[3][2]);
}

RC_TEST("no fault gets past all of the checks") {
  const std::vector<std::vector<bool> > caught = detection_matrix(checks(), runs_of(faults()));
  const std::vector<std::size_t> missed = faults_missed(caught);
  std::cout << "  faults missed by every check: " << missed.size() << "\n";
  RC_CHECK(missed.empty());

  // And the function does say so when one is missed.
  RC_CHECK(caught.size() == 6);
  if (caught.size() != 6) return;
  std::vector<std::vector<bool> > holey = caught;
  std::fill(holey[2].begin(), holey[2].end(), false);
  const std::vector<std::size_t> now = faults_missed(holey);
  RC_CHECK(now.size() == 1);
  if (!(now.size() == 1)) return;
  RC_CHECK_EQ(now[0], static_cast<std::size_t>(2));
}

RC_TEST("three of the six faults depend on a single check, and taking that check away hides them") {
  const std::vector<std::vector<bool> > caught = detection_matrix(checks(), runs_of(faults()));
  const std::vector<std::size_t> alone = faults_with_one_witness(caught);
  std::cout << "  faults seen by exactly one check:";
  const std::vector<Fault> fs = faults();
  for (std::size_t f : alone) std::cout << " " << name_of(fs, f) << ",";
  std::cout << "\n";
  RC_CHECK(alone.size() == 3);
  if (!(alone.size() == 3)) return;
  RC_CHECK_EQ(alone[0], static_cast<std::size_t>(0));        // sign flipped: the behaviour check only
  RC_CHECK_EQ(alone[1], static_cast<std::size_t>(2));        // weak actuator: the behaviour check only
  RC_CHECK_EQ(alone[2], static_cast<std::size_t>(3));        // limiter removed: the limit check only

  std::cout << "  faults no longer caught if a check is removed:\n";
  for (std::size_t c = 0; c < 3; ++c) std::cout << "    without \"" << kCheckNames[c] << "\": " << unseen_without(caught, c) << "\n";
  std::cout << "\n";
  RC_CHECK_EQ(unseen_without(caught, 0), 2);                  // the behaviour check: sign flipped and weak actuator
  RC_CHECK_EQ(unseen_without(caught, 1), 1);                  // the limit check: the limiter
  RC_CHECK_EQ(unseen_without(caught, 2), 0);                  // the model check duplicates what the behaviour check sees, and says why
}

RC_TEST("a check that is too loose lets a fault through, and the table shows it") {
  // Loosen the behaviour check so that it accepts anything that ends within 0.6 of the target. The weak actuator ends 0.2
  // away, so it is accepted; the limit check and the model check do not see it either.
  const std::vector<Check> loose = {
      [](const Run& run) { return std::fabs(run.y.back() - run.target) < 0.6; },
      output_within_limit,
      sensor_agrees_with_model,
  };
  const std::vector<Fault> fs = faults();
  const std::vector<std::vector<bool> > caught = detection_matrix(loose, runs_of(fs));
  const std::vector<std::size_t> missed = faults_missed(caught);
  std::cout << "  with the behaviour check loosened to \"ends within 0.6 of the target\", faults missed:";
  for (std::size_t f : missed) std::cout << " " << name_of(fs, f) << ",";
  std::cout << "\n";
  RC_CHECK(missed.size() == 1);
  if (!(missed.size() == 1)) return;
  RC_CHECK_EQ(missed[0], static_cast<std::size_t>(2));       // the weak actuator
  const std::size_t singles = faults_with_one_witness(caught).size();
  std::cout << "  and " << singles << " of the 6 faults are now seen by exactly one check, where there were 3\n\n";
  RC_CHECK_EQ(singles, static_cast<std::size_t>(4));
}
