// rc/sim/judge.hpp
//
// Judging a recorded run step by step, from lesson 18-11, graduated.
//
// For every change of target in a trace: the overshoot as a fraction of the step's size, the rise time from 10 to 90 percent, the settling time into a band (-1 if it never settled) and the final error, measured the same for a step down as for a step up. A step of no size is left out. A verdict
// names each step and each criterion broken, like "step at 2.5 s: overshoot", and is empty for a pass.
//
// A first order lag of 0.2 s gave overshoot 0.0000, rise 0.4400 s (tau ln 9 is 0.4394) and settling 0.7900 s (tau ln 50 is 0.7824). An underdamped step up and its mirror down gave identical overshoot 0.0759, rise 0.1800 and settling 0.5600. A position loop through a scenario of three steps had overshoots of 0.253, 0.249 and 0.249 and
// settling of 0.640 s each and passed criteria of 0.3, 1.0 s and 0.02; the same loop with a sensor 60 ms late failed every step and with a sensor stuck from 1 s failed all but the overshoot of the last.

#ifndef RC_SIM_JUDGE_HPP
#define RC_SIM_JUDGE_HPP

#include <cmath>
#include <cstddef>
#include <string>
#include <vector>

namespace rc {
namespace sim {

// A run, recorded every `dt` seconds.
struct Trace {
  double dt = 0.01;
  std::vector<double> target;
  std::vector<double> position;
};

// What happened after one change of target.
struct StepMetrics {
  double at = 0.0;               // when the target changed
  double from = 0.0;             // where the position was just before
  double to = 0.0;               // the new target
  double overshoot = 0.0;        // how far past the new target it went, as a fraction of the size of the step; zero if it never passed it
  double rise_time = -1.0;       // from 10 to 90 percent of the way, or -1 if it did not get there
  double settling_time = -1.0;   // from the change until it stayed within `band` of the new target for the rest of the step, or -1 if it never did
  double final_error = 0.0;      // new target minus position at the end of the step
};

// Measure what happened after each of the changes of target in `change_times` (seconds, in order). A step runs from its change to the next change, or the end of the trace.
// A step of no size (the target did not really move from where the position was) is left out. Steps down are measured the same way as steps up: the sign of the step
// says which direction is "past" the target. `band` is the fraction of the step's size that counts as settled.
inline std::vector<StepMetrics> analyse_steps(const Trace& trace, const std::vector<double>& change_times, double band) {
  std::vector<StepMetrics> out;
  const std::size_t n = trace.position.size() < trace.target.size() ? trace.position.size() : trace.target.size();
  for (std::size_t k = 0; k < change_times.size(); ++k) {
    const long start = static_cast<long>(std::lround(change_times[k] / trace.dt));
    const long end_index = k + 1 < change_times.size() ? static_cast<long>(std::lround(change_times[k + 1] / trace.dt)) : static_cast<long>(n);
    if (start < 0 || start >= static_cast<long>(n)) continue;
    const long end = end_index > static_cast<long>(n) ? static_cast<long>(n) : end_index;
    if (end <= start + 1) continue;

    StepMetrics m;
    m.at = change_times[k];
    m.from = start > 0 ? trace.position[static_cast<std::size_t>(start - 1)] : trace.position[static_cast<std::size_t>(start)];
    m.to = trace.target[static_cast<std::size_t>(start)];
    const double size = m.to - m.from;
    if (std::fabs(size) < 1e-6) continue;
    const double sign = size > 0.0 ? 1.0 : -1.0;

    double peak = 0.0;
    double reached_10 = -1.0;
    long last_outside = -1;
    for (long i = start; i < end; ++i) {
      const double p = trace.position[static_cast<std::size_t>(i)];
      const double progress = (p - m.from) / size;
      const double t = (i - start) * trace.dt;
      if (reached_10 < 0.0 && progress >= 0.1) reached_10 = t;
      if (m.rise_time < 0.0 && reached_10 >= 0.0 && progress >= 0.9) m.rise_time = t - reached_10;
      const double past = (p - m.to) * sign;
      if (past > peak) peak = past;
      if (std::fabs(p - m.to) > band * std::fabs(size)) last_outside = i;
    }
    m.overshoot = peak / std::fabs(size);
    if (last_outside < 0) m.settling_time = 0.0;
    else if (last_outside + 1 < end) m.settling_time = (last_outside + 1 - start) * trace.dt;
    m.final_error = m.to - trace.position[static_cast<std::size_t>(end - 1)];
    out.push_back(m);
  }
  return out;
}

// What a run has to achieve.
struct Criteria {
  double max_overshoot = 0.1;      // as a fraction of the step
  double max_settling = 1.0;       // seconds
  double max_final_error = 0.02;   // in the units of the position
};

// The reasons a run failed, one for each step and each criterion it broke, in step order, like "step at 2.5 s: overshoot".
// A step that never settled fails "settling". An empty answer is a pass, and so is a run with no steps in it, which is not a judgement of anything: see `steps`.
inline std::vector<std::string> verdict(const std::vector<StepMetrics>& steps, const Criteria& c) {
  std::vector<std::string> failures;
  for (const StepMetrics& s : steps) {
    const std::string where = "step at " + std::to_string(s.at).substr(0, std::to_string(s.at).find('.') + 2) + " s: ";
    if (s.overshoot > c.max_overshoot) failures.push_back(where + "overshoot");
    if (s.settling_time < 0.0 || s.settling_time > c.max_settling) failures.push_back(where + "settling");
    if (std::fabs(s.final_error) > c.max_final_error) failures.push_back(where + "final error");
  }
  return failures;
}

}  // namespace sim
}  // namespace rc

#endif  // RC_SIM_JUDGE_HPP
