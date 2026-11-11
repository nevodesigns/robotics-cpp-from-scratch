#include <rc/test/rc_test.hpp>
#include <rc/sim/stepper.hpp>

#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

#include "solution.hpp"

namespace {

const char* kGood =
    "# a step, a push and a sensor fault\n"
    "at 0.0 target 1.0\n"
    "\n"
    "at 2.5 target -0.5   # swing the other way\n"
    "at 4.0 disturb 3.0\n"
    "at 6.0 fault sensor stuck\n"
    "at 7.0 fault sensor clear\n"
    "at 10 end\n";

}  // namespace

RC_TEST("a script is read into events with their times, kinds, values and the line each came from, and comments and blank lines are left out") {
  Scenario s;
  const ParseResult r = parse_scenario(kGood, s);
  std::cout << "  a script of 7 lines with a comment and a blank: " << (r.ok ? "ok" : "refused") << ", " << s.events.size() << " events, lasting " << s.duration() << " s\n\n";
  RC_CHECK(r.ok);
  RC_CHECK(s.events.size() == 6);
  if (s.events.size() != 6) return;
  RC_CHECK(s.events[0].kind == ScenarioEvent::Kind::target && s.events[0].value == 1.0 && s.events[0].line == 2);
  RC_CHECK(s.events[1].kind == ScenarioEvent::Kind::target && s.events[1].value == -0.5 && s.events[1].time == 2.5 && s.events[1].line == 4);
  RC_CHECK(s.events[2].kind == ScenarioEvent::Kind::disturb && s.events[2].value == 3.0);
  RC_CHECK(s.events[3].kind == ScenarioEvent::Kind::fault && s.events[3].text == "sensor stuck");
  RC_CHECK(s.events[4].text == "sensor clear");
  RC_CHECK(s.events[5].kind == ScenarioEvent::Kind::end && s.events[5].time == 10.0);
  RC_CHECK_NEAR(s.duration(), 10.0, 1e-12);
  Scenario noeol;
  RC_CHECK(parse_scenario("at 1 target 2\nat 2 end", noeol).ok);        // no newline at the end of the last line
  RC_CHECK(noeol.events.size() == 2);
}

RC_TEST("every bad line is reported with its number, the parse goes on to find the rest, and nothing is left behind") {
  const std::string bad =
      "at 0 target 1\n"
      "at x target 2\n"            // 2: not a time
      "at 1 jump 3\n"              // 3: unknown
      "at 2 target\n"              // 4: no number
      "at 3 target 1 2\n"          // 5: two numbers
      "at -1 target 1\n"           // 6: negative
      "at 5 target 1\n"
      "at 4 target 1\n"            // 8: backwards
      "at 6 fault\n"               // 9: nothing said
      "at 7 end now\n"             // 10: end takes nothing
      "set 8 target 1\n"           // 11: does not start with at
      "at 9 target 1.5e\n"         // 12: not a number
      "at 10 end\n"
      "at 11 target 1\n";          // 14: after the end
  Scenario s;
  s.events.push_back(ScenarioEvent());                  // something left over from before
  const ParseResult r = parse_scenario(bad, s);
  std::cout << "  bad lines found:";
  for (const ScenarioError& e : r.errors) std::cout << " " << e.line;
  std::cout << "\n  first: line " << (r.errors.empty() ? 0 : r.errors[0].line) << ": " << (r.errors.empty() ? "" : r.errors[0].message) << "\n\n";
  RC_CHECK(!r.ok);
  RC_CHECK(s.events.empty());
  RC_CHECK(r.errors.size() == 11);
  if (r.errors.size() != 11) return;
  const int expected[] = {2, 3, 4, 5, 6, 8, 9, 10, 11, 12, 14};
  for (int i = 0; i < 11; ++i) RC_CHECK(r.errors[static_cast<std::size_t>(i)].line == expected[i]);
  RC_CHECK(r.errors[0].message.find("not a number") != std::string::npos);
}

RC_TEST("a script with no end, two ends, or events after its end is refused, and an empty text has no end either") {
  Scenario s;
  const ParseResult none = parse_scenario("at 0 target 1\nat 3 target 2\n", s);
  const ParseResult two = parse_scenario("at 0 target 1\nat 3 end\nat 4 end\n", s);
  const ParseResult empty = parse_scenario("", s);
  const ParseResult only_comments = parse_scenario("# nothing here\n\n", s);
  std::cout << "  no end: " << (none.ok ? "ok" : "refused") << " (" << (none.errors.empty() ? "" : none.errors.back().message) << "); two ends: " << (two.ok ? "ok" : "refused")
            << "; empty: " << (empty.ok ? "ok" : "refused") << "\n\n";
  RC_CHECK(!none.ok);
  RC_CHECK(none.errors.size() == 1);
  RC_CHECK(none.errors.size() == 1 && none.errors[0].message == "the scenario has no end");
  RC_CHECK(!two.ok);
  RC_CHECK(!empty.ok);
  RC_CHECK(!only_comments.ok);
  RC_CHECK(parse_scenario("at 0 end\n", s).ok);
  RC_CHECK(s.events.size() == 1);
}

RC_TEST("stepping through a scenario delivers every event once and none twice, and an event on a step boundary is delivered by the step that ends there") {
  Scenario s;
  RC_CHECK(parse_scenario(kGood, s).ok);
  const double dt = 0.01;
  rc::sim::FixedStepper stepper(dt, 100);
  std::vector<int> delivered_at_step;
  std::vector<int> count(s.events.size(), 0);
  int steps = 0;
  double t0 = -1.0;                                      // the first interval starts before zero, so that an event at time zero is inside it
  while (steps < 1100) {
    const int n = stepper.advance(dt);
    for (int k = 0; k < n; ++k) {
      ++steps;
      const double t1 = steps * dt;
      for (const ScenarioEvent& e : events_between(s, t0, t1))
        for (std::size_t i = 0; i < s.events.size(); ++i)
          if (s.events[i].line == e.line) {
            ++count[i];
            delivered_at_step.push_back(steps);
          }
      t0 = t1;
    }
  }
  bool each_once = true;
  for (std::size_t i = 0; i < count.size(); ++i)
    if (count[i] != 1) each_once = false;
  std::cout << "  stepping 10 ms for 11 s: each of the " << s.events.size() << " events delivered once: " << (each_once ? "yes" : "no") << "; at steps";
  for (int st : delivered_at_step) std::cout << " " << st;
  std::cout << "\n\n";
  RC_CHECK(each_once);
  RC_CHECK(delivered_at_step.size() == 6);
  RC_CHECK(delivered_at_step.size() == 6 && delivered_at_step[0] == 1 && delivered_at_step[1] == 250);      // the event at 2.5 s, by the step that ends at 2.5 s
  RC_CHECK(events_between(s, 2.5, 2.6).empty());                                // not again by the next step
  RC_CHECK(events_between(s, 2.49, 2.5).size() == 1);
  // An interval that starts at zero leaves out an event at zero: the first one must start earlier.
  RC_CHECK(events_between(s, 0.0, 0.01).empty());
  RC_CHECK(events_between(s, -1.0, 0.01).size() == 1);
}

RC_TEST("events at the same time arrive together in the order they were written, and a time that is built up by adding is one step late where one that is multiplied is not") {
  Scenario s;
  RC_CHECK(parse_scenario("at 1.0 target 2\nat 1.0 disturb 5\nat 1.0 fault sensor stuck\nat 2.0 end\n", s).ok);
  const std::vector<ScenarioEvent> together = events_between(s, 0.99, 1.0);
  RC_CHECK(together.size() == 3);
  RC_CHECK(together.size() == 3 && together[0].kind == ScenarioEvent::Kind::target && together[1].kind == ScenarioEvent::Kind::disturb && together[2].kind == ScenarioEvent::Kind::fault);

  // Where does the event at 1.0 s arrive, with steps of 0.1 s: when the time of each step is worked out by adding, and when by multiplying?
  int added_step = -1;
  int multiplied_step = -1;
  double t_added = 0.0;
  double previous_added = 0.0;
  for (int i = 1; i <= 20; ++i) {
    t_added += 0.1;
    if (added_step < 0 && !events_between(s, previous_added, t_added).empty()) added_step = i;
    previous_added = t_added;
  }
  double previous_mult = 0.0;
  for (int i = 1; i <= 20; ++i) {
    const double t = i * 0.1;
    if (multiplied_step < 0 && !events_between(s, previous_mult, t).empty()) multiplied_step = i;
    previous_mult = t;
  }
  std::cout << "  an event at 1.0 s with steps of 0.1 s: the time of each step built up by adding 0.1 delivers it at step " << added_step << ", worked out as step number times 0.1 at step "
            << multiplied_step << "\n\n";
  RC_CHECK(multiplied_step == 10);
  RC_CHECK(added_step == 11);
}
