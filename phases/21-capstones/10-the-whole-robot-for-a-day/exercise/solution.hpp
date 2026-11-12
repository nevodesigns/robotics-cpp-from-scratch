#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <iomanip>
#include <rc/capstone/delivery_rover.hpp>
#include <rc/capstone/ship_gate.hpp>
#include <rc/capstone/test_report.hpp>
#include <rc/safety/blackbox.hpp>
#include <rc/safety/supervisor.hpp>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

// How many of the rows are jobs that reached the goal, and how many were stopped by the supervisor (a row with any stop cause).
inline int count_delivered(const std::vector<rc::capstone::SuiteRow>& rows) {
  // TODO: the number of rows whose result reached the goal
  (void)rows;
  return 0;
}

inline int count_stopped(const std::vector<rc::capstone::SuiteRow>& rows) {
  // TODO: the number of rows whose result has a stop cause (causes not 0)
  (void)rows;
  return 0;
}

struct DayResult {
  std::string version;
  bool gate_passed = false;
  int jobs = 0;
  int delivered = 0;
  int stopped = 0;                      // jobs ended by a stop of the supervisor
  int bumps = 0;                        // ticks of contact, over the day
  int operator_resets = 0;              // a latched stop needs a person: one for each stopped job
  unsigned causes_seen = 0;             // the stop causes of the day, together
  std::size_t log_events = 0;
  bool log_verified = false;
  std::vector<std::pair<int, double>> before_first_stop;   // the code and value of the events in the black box just before the first stop (up to six)
  std::vector<std::string> uncovered;   // the hazards of class medium or worse that no scenario is evidence for
  std::string report;
};

// The report of a day, as text, in this form (every line ending in a newline):
//   Rover <version> (gate <passed or REFUSED>)
//   Day: <jobs> jobs, <delivered> delivered, <stopped> stopped by the supervisor (<causes_text of causes_seen>), <bumps> bumps, <operator_resets> operator resets
//   Black box: <log_events> events, chain <verified or BROKEN>
//   Before the first stop: <code>:<value> for each event, joined by a space, the value with one decimal   (or "Before the first stop: none" if there was no stop)
//   Open: hazards with no scenario: <the ids joined by ", ">                                           (or "Open: nothing uncovered" if there are none)
inline std::string render_day(const DayResult& d) {
  // TODO: the five lines in the comment above, exactly, with std::ostringstream; the causes by rc::safety::causes_text; the events as code:value with one decimal (std::fixed, std::setprecision(1))
  (void)d;
  return std::string();
}

// A working day of the rover under a policy: the jobs are the five scenarios of lesson 21-08 in order (an empty building, somebody steps out, somebody in the doorway, the link lost, the sensor dead), each run in turn with the policy. A black box (lesson 19-12, 64 events) is kept on the day's own clock, the seconds
// run so far: for each job an event of code 1 (the job number, from 0) when it starts, and when it ends code 2 with the time it took if it reached the goal, code 99 with the stop causes if the supervisor stopped it, otherwise code 4 with 0; and code 3 with the bumps if there were any, before the end event.
// The day's totals come from the rows; the log is dumped and verified; the six events before the first code 99 are kept; the hazards with no scenario are listed (hazard_coverage of the five scenarios against rover_hazards); and the report is made by render_day.
inline DayResult run_day(const rc::capstone::DeliveryConfig& policy, const std::string& version, bool gate_passed) {
  DayResult d;
  d.version = version;
  d.gate_passed = gate_passed;
  const std::vector<rc::capstone::SuiteScenario> scenarios = rc::capstone::default_suite(policy);
  const std::vector<rc::capstone::SuiteRow> rows = rc::capstone::run_suite(scenarios);
  rc::safety::BlackBox box(64);
  double clock = 0.0;
  for (std::size_t i = 0; i < rows.size(); ++i) {
    const rc::capstone::DeliveryResult& r = rows[i].result;
    box.record(clock, 1, static_cast<double>(i));
    clock += r.time;
    if (r.bumps > 0) box.record(clock, 3, static_cast<double>(r.bumps));
    if (r.causes != 0) box.record(clock, 99, static_cast<double>(r.causes));
    else if (r.reached) box.record(clock, 2, r.time);
    else box.record(clock, 4, 0.0);
    d.bumps += r.bumps;
    d.causes_seen |= r.causes;
  }
  d.jobs = static_cast<int>(rows.size());
  d.delivered = count_delivered(rows);
  d.stopped = count_stopped(rows);
  d.operator_resets = d.stopped;
  const rc::safety::Dump dump = box.dump();
  d.log_events = dump.events.size();
  d.log_verified = rc::safety::verify(dump).ok;
  for (const rc::safety::Event& e : rc::safety::window_before(dump, 99, 6)) d.before_first_stop.push_back({e.code, e.value});
  for (const rc::capstone::HazardLine& h : rc::capstone::hazard_coverage(rc::capstone::rover_hazards(), scenarios, rows))
    if (h.status == rc::capstone::CoverageStatus::uncovered) d.uncovered.push_back(h.id);
  d.report = render_day(d);
  return d;
}

#endif  // LESSON_SOLUTION_HPP
