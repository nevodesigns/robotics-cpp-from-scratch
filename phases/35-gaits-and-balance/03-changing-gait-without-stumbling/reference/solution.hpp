#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cmath>
#include <rc/legged/stance_swing.hpp>

using namespace rc::legged;

enum GaitKind { kWalk = 0, kTrot = 1, kPace = 2, kBound = 3 };

// The phase offsets of the four legs (front left, front right, rear left, rear right).
inline void gait_offsets(int kind, double out[4]) {
  static const double table[4][4] = {{0.0, 0.5, 0.25, 0.75},   // walk: one foot at a time, left side then right
                                     {0.0, 0.5, 0.5, 0.0},     // trot: the diagonal pairs together
                                     {0.0, 0.5, 0.0, 0.5},     // pace: the pairs on one side together
                                     {0.0, 0.0, 0.5, 0.5}};    // bound: the front pair, then the rear pair
  for (int i = 0; i < 4; ++i) out[i] = table[kind][i];
}

// Moves each offset toward its target by at most rate * dt, by the shortest way round the cycle.
inline void step_offsets_toward(double cur[4], const double target[4], double rate, double dt) {
  const double max_step = rate * dt;
  for (int i = 0; i < 4; ++i) {
    double d = target[i] - cur[i];
    d -= std::floor(d + 0.5);                 // the shortest way round: -0.5 to 0.5
    if (d > max_step) d = max_step;
    if (d < -max_step) d = -max_step;
    cur[i] += d;
    cur[i] -= std::floor(cur[i]);
  }
}

// The gait for a speed, from the one it was: up to the trot above v_up, back to the walk only below v_down.
inline int gait_for_speed(double v, int current, double v_up, double v_down) {
  if (current == kWalk && v > v_up) return kTrot;
  if (current == kTrot && v < v_down) return kWalk;
  return current;
}

// The fewest feet on the ground over a time while the gait changes from `from` to `to` at `rate` cycles per second (a very large rate is an abrupt change), the change starting at time t_switch; the duty is the same throughout. The time the change took is in `t_change`.
inline int min_feet_down(int from, int to, double rate, double t_switch, double duty, double period, double* t_change) {
  GaitTiming g;
  g.duty = duty;
  g.period = period;
  double target[4];
  gait_offsets(to, target);
  gait_offsets(from, g.offset);
  double cur[4];
  for (int i = 0; i < 4; ++i) cur[i] = g.offset[i];
  int least = 4;
  const double dt = 0.001;
  double changed_at = -1.0;
  for (double t = 0.0; t < t_switch + 2.0; t += dt) {
    if (t >= t_switch) {
      step_offsets_toward(cur, target, rate, dt);
      bool done = true;
      for (int i = 0; i < 4; ++i)
        if (std::fabs(cur[i] - target[i]) > 1e-9 && std::fabs(std::fabs(cur[i] - target[i]) - 1.0) > 1e-9) done = false;
      if (done && changed_at < 0.0) changed_at = t - t_switch;
    }
    // the phase of a leg is the time's fraction of the period plus its offset, wrapped; the offsets are changed in the moving frame, so what counts is the present offset
    for (int i = 0; i < 4; ++i) g.offset[i] = cur[i];
    // count the feet on the ground by the phase at this time with these offsets
    const int n = count_stance(g, t);
    if (n < least) least = n;
  }
  if (t_change) *t_change = changed_at;
  return least;
}


#endif
