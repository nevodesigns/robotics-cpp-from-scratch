#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cmath>
#include <cstddef>
#include <string>
#include <vector>

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
  // TODO: n = the smaller of the two trace lengths. For each change time: start = the sample index round(time / dt); the step runs to the next change time's index or to n, whichever applies (never beyond n); skip a step with no room (end <= start + 1) or a start outside the trace.
  // from = the position at the sample just before start (at start if there is none); to = the target at start; size = to - from; skip a step with |size| below 1e-6. sign = +1 for a step up, -1 for a step down.
  // Over the samples of the step: progress = (position - from) / size. rise_time = the time from first reaching progress 0.1 to first reaching 0.9, or -1. past = (position - to) * sign; the largest past is the peak; overshoot = peak / |size|.
  // settling: the last sample whose |position - to| is over band * |size|; no such sample: 0; the last sample of the step: -1 (never settled); otherwise (that index + 1 - start) * dt. final_error = to - the position at the last sample of the step.
  // at = the change time. Return the list of steps measured.
  (void)trace;
  (void)change_times;
  (void)band;
  return std::vector<StepMetrics>();
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
  // TODO: For each step in order, for each of these that applies add a reason beginning \"step at T s: \" with T the time to one decimal place (for example \"step at 2.5 s: overshoot\"):
  // \"overshoot\" if the overshoot is over max_overshoot; \"settling\" if settling_time is negative (never settled) or over max_settling; \"final error\" if the absolute final error is over max_final_error.
  (void)steps;
  (void)c;
  return std::vector<std::string>();
}

#endif  // LESSON_SOLUTION_HPP
