#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cmath>

// What a run of the sampled loop did.
struct LoopResult {
  bool stable = false;        // still within 5 percent of the target over the last tenth of the run
  double overshoot = 0.0;     // how far past the target it went, as a fraction of it
  double final_error = 0.0;   // target minus position at the end
};

// A unit mass with damping 0.2, pushed by a controller that runs once every `period` seconds.
inline LoopResult run_sampled(double period, double kp, double kd) {
  // TODO: the plant moves in steps of h = 1e-4 s for 60000 steps (6 s). The target is 1.
  //
  // Every `per` steps, where per = round(period / h) and at least 1, the controller ticks: it reads the
  // position and velocity and sets force = kp * (1 - position) - kd * velocity. The force is held until the next tick.
  // Every step: velocity += (force - 0.2 * velocity) * h; position += velocity * h.
  //
  // Track the peak position and the largest |1 - position| over the last tenth of the run.
  // If |position| ever reaches 1e6, or is not a number, return the default result at once.
  // stable = that largest error is under 0.05; overshoot = peak - 1 (never below 0); final_error = 1 - position.
  (void)period;
  (void)kp;
  (void)kd;
  return LoopResult{};
}

// The longest period, found by bisection between 1 ms and 2 s, at which the loop is still stable.
inline double largest_stable_period(double kp, double kd) {
  // TODO: lo = 0.001, hi = 2.0. If run_sampled(lo, ...) is not stable, return 0.
  // Then 24 times: mid = (lo + hi) / 2; if the loop is stable at mid, lo = mid, otherwise hi = mid. Return lo.
  (void)kp;
  (void)kd;
  return 0.0;
}

// A period chosen as `share` of the largest stable one. Zero or less gives zero.
inline double period_for(double kp, double kd, double share) {
  // TODO: share * largest_stable_period(kp, kd), or 0 for a share of zero or less.
  (void)kp;
  (void)kd;
  (void)share;
  return 0.0;
}

#endif  // LESSON_SOLUTION_HPP
