#include <rc/test/rc_test.hpp>

#include <cmath>
#include <iomanip>
#include <iostream>
#include <stdexcept>

#include "solution.hpp"

namespace {

// A contactor that can cut the motor supply: it opens when it is commanded open, unless its contacts have welded.
struct Contactor {
  bool stuck_closed = false;
  bool commanded_closed = true;
  void command(bool close) { commanded_closed = close; }
  bool closed() const { return stuck_closed ? true : commanded_closed; }     // what an auxiliary contact on it would read
};

// The stop logic, in software: the supply must be commanded open when the loop is open.
bool supply_command_when_loop_open() { return false; }

std::string joined(const std::vector<std::string>& v) {
  std::string s;
  for (const std::string& x : v) s += (s.empty() ? "" : ", ") + x;
  return s.empty() ? "none" : s;
}

}  // namespace

RC_TEST("every check is run, a failure does not hide the next, and a throw is a failure") {
  SelfTest t;
  t.add("heartbeat input", [] { return true; });
  t.add("e-stop loop reads", [] { return false; });
  t.add("battery sensor", []() -> bool { throw std::runtime_error("no reply"); });
  t.add("tilt sensor", [] { return true; });
  t.add("fence loaded", std::function<bool()>());
  const std::vector<CheckResult> r = t.run();
  std::cout << "  five checks, run in order:\n";
  for (const CheckResult& c : r) std::cout << "  " << (c.passed ? "pass" : "FAIL") << "  " << c.name << (c.detail.empty() ? "" : "  (" + c.detail + ")") << "\n";
  std::cout << "  failed: " << joined(failed_names(r)) << "\n\n";
  RC_CHECK(r.size() == 5);
  if (r.size() != 5) return;
  RC_CHECK(r[0].passed && r[3].passed);
  RC_CHECK(!r[1].passed && r[1].detail == "returned false");
  RC_CHECK(!r[2].passed && r[2].detail == "threw an exception");
  RC_CHECK(!r[4].passed && r[4].detail == "no check");
  RC_CHECK(!all_passed(r));
  RC_CHECK(failed_names(r).size() == 3);
  RC_CHECK(r[3].name == "tilt sensor");
}

RC_TEST("a test that checked nothing has not passed") {
  SelfTest empty;
  RC_CHECK(empty.size() == 0);
  RC_CHECK(!all_passed(empty.run()));
  SelfTest ok;
  ok.add("one", [] { return true; });
  ok.add("two", [] { return true; });
  RC_CHECK(all_passed(ok.run()));
  std::cout << "  an empty test list is not a pass; two passing checks are\n\n";
}

RC_TEST("permission to drive needs a test that passed, lasts an interval, and is lost by a failed test or a clock that goes back") {
  Permit p(3600.0);
  RC_CHECK(!p.allowed(0.0));                       // never tested
  p.record(100.0, true);
  RC_CHECK(p.allowed(100.0));
  RC_CHECK(p.allowed(3700.0));                     // exactly at the interval
  RC_CHECK(!p.allowed(3700.5));                    // just past it
  RC_CHECK(!p.allowed(50.0));                      // the clock went back
  p.record(2000.0, false);
  RC_CHECK(!p.allowed(2001.0));                    // a failed test cancels the earlier pass
  p.record(2100.0, true);
  RC_CHECK(p.allowed(2200.0));
  std::cout << "  permitted at 100 s and for 3600 s after, refused at 3700.5 s, refused after a failed test at 2000 s, and again permitted after a pass at 2100 s\n\n";
}

RC_TEST("a check that only reads the software's answer passes with the contactor welded, and one that asks the contactor does not") {
  Contactor healthy;
  Contactor welded;
  welded.stuck_closed = true;
  // The software check: the stop logic says to open the supply.
  auto software_says_open = [] { return supply_command_when_loop_open() == false; };
  // The check that tests the stop: open the loop, command the contactor open, and read its auxiliary contact.
  auto contactor_really_open = [](Contactor c) {
    c.command(false);
    return !c.closed();
  };
  SelfTest on_healthy;
  on_healthy.add("stop logic opens the supply", software_says_open);
  on_healthy.add("contactor opens", [&] { return contactor_really_open(healthy); });
  SelfTest on_welded;
  on_welded.add("stop logic opens the supply", software_says_open);
  on_welded.add("contactor opens", [&] { return contactor_really_open(welded); });
  const std::vector<CheckResult> a = on_healthy.run();
  const std::vector<CheckResult> b = on_welded.run();
  RC_CHECK(a.size() == 2 && b.size() == 2);
  if (a.size() != 2 || b.size() != 2) return;
  std::cout << "  contactor welded closed:  the software check " << (b[0].passed ? "passes" : "fails") << ", the check that reads the contactor " << (b[1].passed ? "passes" : "FAILS") << "\n";
  std::cout << "  contactor healthy:        both " << (all_passed(a) ? "pass" : "do not both pass") << "\n\n";
  RC_CHECK(all_passed(a));
  RC_CHECK(b[0].passed);
  RC_CHECK(!b[1].passed);
  RC_CHECK(!all_passed(b));
}

RC_TEST("the chance of an undetected failure grows with the time since the last test, and the interval follows from a target") {
  const double rate = 1e-5;                        // one dangerous failure in 100000 hours, about eleven years
  std::cout << "  a failure rate of 1e-5 per hour: chance that it has failed since the last test\n  time since the test   chance\n";
  const struct { const char* what; double hours; } rows[] = {{"1 hour", 1.0}, {"1 day", 24.0}, {"1 week", 168.0}, {"1 month", 730.0}, {"1 year", 8766.0}};
  double previous = 0.0;
  for (const auto& r : rows) {
    const double p = failure_probability(rate, r.hours);
    std::cout << "  " << r.what << std::string(18 - std::string(r.what).size(), ' ') << std::scientific << std::setprecision(2) << p << "\n";
    RC_CHECK(p > previous);
    previous = p;
  }
  RC_CHECK_NEAR(failure_probability(rate, 8766.0), 0.0839, 1e-3);
  const double h = max_interval(rate, 1e-3);
  std::cout << "  to keep the chance at or below 1 in 1000: test every " << std::fixed << std::setprecision(1) << h << " hours (" << std::setprecision(1) << h / 24.0 << " days); at a rate of 1e-6: every " << std::setprecision(0) << max_interval(1e-6, 1e-3) << " hours\n\n";
  RC_CHECK_NEAR(failure_probability(rate, h), 1e-3, 1e-12);
  RC_CHECK_NEAR(h, 100.05, 0.01);
  RC_CHECK_NEAR(max_interval(1e-6, 1e-3), 1000.5, 0.1);
  RC_CHECK(failure_probability(0.0, 100.0) == 0.0);
  RC_CHECK(failure_probability(rate, -5.0) == 0.0);
  RC_CHECK(std::isinf(max_interval(0.0, 1e-3)));
  RC_CHECK(max_interval(rate, 0.0) == 0.0);
}
