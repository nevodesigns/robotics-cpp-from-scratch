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
//
// The plant moves in steps of 0.1 ms. The controller reads position and speed only at its own ticks,
// computes force = kp * (1 - position) - kd * velocity, and the motor holds that force until the next tick.
// The target is 1 and the run lasts 6 seconds.
inline LoopResult run_sampled(double period, double kp, double kd) {
  const double h = 1e-4;
  const double damping = 0.2;
  const int steps = 60000;
  long per = static_cast<long>(std::lround(period / h));
  if (per < 1) per = 1;

  double position = 0.0;
  double velocity = 0.0;
  double force = 0.0;
  double peak = 0.0;
  double worst_tail = 0.0;
  LoopResult result;
  for (int i = 0; i < steps; ++i) {
    if (i % per == 0) force = kp * (1.0 - position) - kd * velocity;
    velocity += (force - damping * velocity) * h;
    position += velocity * h;
    if (position > peak) peak = position;
    if (!(std::fabs(position) < 1e6)) return result;           // blown up, or not a number
    if (i >= steps - steps / 10) {
      const double e = std::fabs(1.0 - position);
      if (e > worst_tail) worst_tail = e;
    }
  }
  result.stable = worst_tail < 0.05;
  result.overshoot = peak > 1.0 ? peak - 1.0 : 0.0;
  result.final_error = 1.0 - position;
  return result;
}

// The longest period, found by bisection between 1 ms and 2 s, at which the loop is still stable.
inline double largest_stable_period(double kp, double kd) {
  double lo = 0.001;
  double hi = 2.0;
  if (!run_sampled(lo, kp, kd).stable) return 0.0;
  for (int i = 0; i < 24; ++i) {
    const double mid = 0.5 * (lo + hi);
    if (run_sampled(mid, kp, kd).stable) lo = mid;
    else hi = mid;
  }
  return lo;
}

// A period chosen as `share` of the largest stable one. Zero or less gives zero.
inline double period_for(double kp, double kd, double share) {
  if (share <= 0.0) return 0.0;
  return share * largest_stable_period(kp, kd);
}

#endif  // LESSON_SOLUTION_HPP
