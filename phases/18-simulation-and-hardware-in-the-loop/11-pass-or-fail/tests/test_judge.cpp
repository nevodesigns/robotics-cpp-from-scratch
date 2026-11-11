#include <rc/test/rc_test.hpp>
#include <rc/sim/faults.hpp>
#include <rc/sim/plant.hpp>
#include <rc/sim/scenario.hpp>
#include <rc/sim/sensor_model.hpp>

#include <cmath>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

#include "solution.hpp"

using rc::sim::Scenario;
using rc::sim::ScenarioEvent;
using rc::sim::events_between;
using rc::sim::parse_scenario;

namespace {

const double kDt = 0.01;

// The response of a first order lag to a step from `from` to `to`, with time constant `tau`, as `seconds` of samples preceded by one sample at `from`.
void append_step(Trace& t, double from, double to, double tau, double seconds) {
  const int n = static_cast<int>(seconds / kDt + 0.5);
  for (int i = 0; i < n; ++i) {
    t.target.push_back(to);
    t.position.push_back(to + (from - to) * std::exp(-(i * kDt) / tau));
  }
}

// A run of the position loop of lesson 18-02 through a scenario of lesson 18-05, with the sensor of 18-04 and a fault from 18-06 if asked.
struct RunSetup {
  int sensor_delay = 0;
  bool stuck_from_one_second = false;
};

Trace run_scenario(const std::string& script, const RunSetup& setup, std::vector<double>& change_times) {
  Scenario s;
  parse_scenario(script, s);
  rc::sim::SimMotor motor(2.0, 0.1, 12.0);
  rc::sim::SensorOptions so;
  so.delay = setup.sensor_delay;
  rc::sim::SensorModel sensor(so);
  rc::sim::FaultInjector fault;
  if (setup.stuck_from_one_second) fault.set(rc::sim::FaultKind::stuck, 1.0, 0.0);
  Trace trace;
  trace.dt = kDt;
  double target = 0.0;
  double measured = 0.0;
  double t0 = -1.0;
  change_times.clear();
  const int steps = static_cast<int>(s.duration() / kDt + 0.5);
  for (int i = 0; i < steps; ++i) {
    const double t1 = i * kDt;
    for (const ScenarioEvent& e : events_between(s, t0, t1))
      if (e.kind == ScenarioEvent::Kind::target) {
        target = e.value;
        change_times.push_back(e.time);
      }
    t0 = t1;
    motor.apply(8.0 * (target - measured), kDt);
    measured = fault.distort(sensor.read(motor.position()).value, (i + 1) * kDt);
    trace.target.push_back(target);
    trace.position.push_back(motor.position());
  }
  return trace;
}

const char* kScript = "at 0 target 1\nat 2.5 target -0.5\nat 5 target 1\nat 8 end\n";

std::string joined(const std::vector<std::string>& v) {
  std::string s;
  for (const std::string& x : v) s += (s.empty() ? "" : "; ") + x;
  return s.empty() ? "none" : s;
}

}  // namespace

RC_TEST("a smooth approach to the target has no overshoot, a rise time of tau times ln 9, and settles in about four time constants") {
  Trace t;
  append_step(t, 0.0, 1.0, 0.2, 3.0);
  const std::vector<StepMetrics> m = analyse_steps(t, {0.0}, 0.02);
  RC_CHECK(m.size() == 1);
  if (m.size() != 1) return;
  std::cout << "  a lag of 0.2 s to a step of 1: overshoot " << std::fixed << std::setprecision(4) << m[0].overshoot << ", rise " << m[0].rise_time << " s (tau ln 9 is " << 0.2 * std::log(9.0)
            << "), settling " << m[0].settling_time << " s (tau ln 50 is " << 0.2 * std::log(50.0) << "), final error " << std::setprecision(5) << m[0].final_error << "\n\n";
  RC_CHECK_NEAR(m[0].overshoot, 0.0, 1e-12);
  RC_CHECK_NEAR(m[0].rise_time, 0.2 * std::log(9.0), 0.02);
  RC_CHECK_NEAR(m[0].settling_time, 0.2 * std::log(50.0), 0.02);
  RC_CHECK(std::fabs(m[0].final_error) < 1e-5);
  RC_CHECK_NEAR(m[0].from, 0.0, 1e-12);
  RC_CHECK_NEAR(m[0].to, 1.0, 1e-12);
}

RC_TEST("a step down is measured as a step up is, and the same shape mirrored gives the same numbers") {
  Trace up;
  Trace down;
  for (int i = 0; i < 300; ++i) {
    const double t = i * kDt;
    const double response = 1.0 - std::exp(-t / 0.15) * (std::cos(8.0 * t) + 0.5 * std::sin(8.0 * t));       // an underdamped step to 1 that overshoots
    up.target.push_back(1.0);
    up.position.push_back(response);
    down.target.push_back(-0.5);
    down.position.push_back(0.5 - 1.0 * response + 0.0);                                                      // from 0.5, 1.0 down to -0.5
  }
  // The down trace starts at 0.5 and its sample before the step is the first sample, so give both a sample before the change.
  const std::vector<StepMetrics> a = analyse_steps(up, {0.0}, 0.02);
  Trace up2;
  Trace down2;
  up2.target.push_back(1.0);
  up2.position.push_back(0.0);
  down2.target.push_back(-0.5);
  down2.position.push_back(0.5);
  for (std::size_t i = 0; i < up.position.size(); ++i) {
    up2.target.push_back(up.target[i]);
    up2.position.push_back(up.position[i]);
    down2.target.push_back(down.target[i]);
    down2.position.push_back(down.position[i]);
  }
  const std::vector<StepMetrics> mu = analyse_steps(up2, {0.01}, 0.02);
  const std::vector<StepMetrics> md = analyse_steps(down2, {0.01}, 0.02);
  RC_CHECK(a.size() == 1);
  RC_CHECK(mu.size() == 1);
  RC_CHECK(md.size() == 1);
  if (mu.size() != 1 || md.size() != 1) return;
  std::cout << "  an underdamped step up from 0 to 1 and the mirror of it down from 0.5 to -0.5\n  direction   size    overshoot   rise (s)   settling (s)\n  up         " << std::fixed
            << std::setprecision(2) << std::setw(5) << (mu[0].to - mu[0].from) << "   " << std::setw(9) << std::setprecision(4) << mu[0].overshoot << "   " << std::setw(8) << mu[0].rise_time << "   "
            << std::setw(12) << mu[0].settling_time << "\n  down       " << std::setprecision(2) << std::setw(5) << (md[0].to - md[0].from) << "   " << std::setw(9) << std::setprecision(4)
            << md[0].overshoot << "   " << std::setw(8) << md[0].rise_time << "   " << std::setw(12) << md[0].settling_time << "\n\n";
  RC_CHECK(mu[0].overshoot > 0.05);
  RC_CHECK_NEAR(md[0].overshoot, mu[0].overshoot, 1e-9);
  RC_CHECK_NEAR(md[0].rise_time, mu[0].rise_time, 1e-9);
  RC_CHECK_NEAR(md[0].settling_time, mu[0].settling_time, 1e-9);
  RC_CHECK(md[0].to - md[0].from < 0.0);
}

RC_TEST("a step of no size is left out, so are changes outside the trace, and each step stops at the next change") {
  Trace t;
  append_step(t, 0.0, 1.0, 0.1, 2.0);
  append_step(t, 1.0, 1.0, 0.1, 2.0);                            // the target 'changes' to where it already is
  append_step(t, 1.0, -1.0, 0.1, 2.0);
  const std::vector<StepMetrics> m = analyse_steps(t, {0.0, 2.0, 4.0, 99.0}, 0.02);
  std::cout << "  four changes, one of no size and one beyond the end: " << m.size() << " steps measured, at";
  for (const StepMetrics& s : m) std::cout << " " << s.at;
  std::cout << " s\n\n";
  RC_CHECK(m.size() == 2);
  if (m.size() != 2) return;
  RC_CHECK(m[0].at == 0.0 && m[1].at == 4.0);
  RC_CHECK_NEAR(m[1].from, 1.0, 1e-9);
  RC_CHECK_NEAR(m[1].to, -1.0, 1e-9);
  // Each step's numbers come only from its own part of the trace: the first step's end is at 2.0 s, where the next change begins.
  RC_CHECK(m[0].settling_time < 1.0);
  RC_CHECK(analyse_steps(Trace(), {0.0}, 0.02).empty());
}

RC_TEST("a run that never stays within the band has no settling time, and the verdict says that and not a number") {
  Trace t;
  for (int i = 0; i < 400; ++i) {
    t.target.push_back(1.0);
    t.position.push_back(1.0 + 0.3 * std::sin(i * 0.2));              // oscillating for ever around the target
  }
  t.position[0] = 0.0;
  const std::vector<StepMetrics> m = analyse_steps(t, {0.01}, 0.02);
  RC_CHECK(m.size() == 1);
  if (m.size() != 1) return;
  const std::vector<std::string> v = verdict(m, Criteria());
  std::cout << "  an oscillation about the target: settling " << m[0].settling_time << ", overshoot " << std::fixed << std::setprecision(2) << m[0].overshoot << "; verdict: " << joined(v) << "\n\n";
  RC_CHECK(m[0].settling_time < 0.0);
  RC_CHECK(m[0].overshoot > 0.2);
  RC_CHECK(joined(v).find("settling") != std::string::npos);
  RC_CHECK(joined(v).find("overshoot") != std::string::npos);
}

RC_TEST("the verdict names each step and each criterion that was broken, in order, and a run that meets them has none") {
  std::vector<StepMetrics> steps(3);
  steps[0].at = 0.0;
  steps[0].overshoot = 0.05;
  steps[0].settling_time = 0.5;
  steps[0].final_error = 0.001;
  steps[1].at = 2.5;
  steps[1].overshoot = 0.40;                                     // too much
  steps[1].settling_time = 0.8;
  steps[1].final_error = 0.001;
  steps[2].at = 5.0;
  steps[2].overshoot = 0.02;
  steps[2].settling_time = 2.0;                                  // too slow
  steps[2].final_error = 0.05;                                   // and not there
  const std::vector<std::string> v = verdict(steps, Criteria());
  std::cout << "  verdict: " << joined(v) << "\n\n";
  RC_CHECK(v.size() == 3);
  if (v.size() != 3) return;
  RC_CHECK(v[0] == "step at 2.5 s: overshoot");
  RC_CHECK(v[1] == "step at 5.0 s: settling");
  RC_CHECK(v[2] == "step at 5.0 s: final error");
  std::vector<StepMetrics> fine(1);
  fine[0].overshoot = 0.0;
  fine[0].settling_time = 0.0;
  fine[0].final_error = 0.0;
  RC_CHECK(verdict(fine, Criteria()).empty());
  Criteria loose;
  loose.max_overshoot = 1.0;
  loose.max_settling = 10.0;
  loose.max_final_error = 1.0;
  RC_CHECK(verdict(steps, loose).empty());
}

RC_TEST("a scenario run on the simulation is judged step by step: the controller passes, a late sensor fails on overshoot and a stuck one fails to arrive") {
  std::vector<double> changes;
  const Criteria criteria{0.3, 1.0, 0.02};
  const Trace good = run_scenario(kScript, RunSetup{0, false}, changes);
  const std::vector<StepMetrics> mg = analyse_steps(good, changes, 0.02);
  RC_CHECK(changes.size() == 3);
  RC_CHECK(mg.size() == 3);
  if (mg.size() != 3) return;
  std::cout << "  scenario: target 1 at 0 s, -0.5 at 2.5 s, 1 at 5 s; position loop kp 8 on the motor of lesson 18-02\n  perfect sensor\n  step at   from    to    overshoot   rise (s)   settling (s)   final error\n";
  for (const StepMetrics& s : mg)
    std::cout << "  " << std::setw(7) << std::fixed << std::setprecision(1) << s.at << "   " << std::setw(5) << std::setprecision(2) << s.from << "  " << std::setw(5) << s.to << "   " << std::setw(9)
              << std::setprecision(3) << s.overshoot << "   " << std::setw(8) << s.rise_time << "   " << std::setw(12) << s.settling_time << "   " << std::setw(11) << std::setprecision(4) << s.final_error << "\n";
  const std::vector<std::string> good_verdict = verdict(mg, criteria);

  const Trace late = run_scenario(kScript, RunSetup{12, false}, changes);
  const std::vector<StepMetrics> ml = analyse_steps(late, changes, 0.02);
  const std::vector<std::string> late_verdict = verdict(ml, criteria);
  const Trace stuck = run_scenario(kScript, RunSetup{0, true}, changes);
  const std::vector<StepMetrics> ms = analyse_steps(stuck, changes, 0.02);
  const std::vector<std::string> stuck_verdict = verdict(ms, criteria);
  std::cout << "  verdicts against overshoot 0.3, settling 1.0 s, final error 0.02\n  perfect sensor:  " << joined(good_verdict) << "\n  sensor 60 ms late: " << joined(late_verdict) << "\n  sensor stuck from 1 s: "
            << joined(stuck_verdict) << "\n\n";
  RC_CHECK(good_verdict.empty());
  RC_CHECK(!late_verdict.empty());
  RC_CHECK(joined(late_verdict).find("overshoot") != std::string::npos);
  RC_CHECK(!stuck_verdict.empty());
  RC_CHECK(joined(stuck_verdict).find("final error") != std::string::npos);
}
