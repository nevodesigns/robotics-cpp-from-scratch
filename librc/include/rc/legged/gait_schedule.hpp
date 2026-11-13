// rc/legged/gait_schedule.hpp
//
// Changing gait without stumbling, from lesson 35-03.
//
// gait_offsets gives the phase offsets of the four legs (front left, front right, rear left, rear right) for a walk (0), a trot (1), a pace (2) or a bound (3). step_offsets_toward moves each current offset toward a target offset by at most a rate (in cycles per second) times a time, by the shortest way round the cycle, because a phase of 0.95 and one of 0.05 are 0.1 apart and not 0.9. gait_for_speed chooses the gait from the speed with hysteresis: a walk changes up to a trot above one speed and back down only below a lower one. min_feet_down is the least number of feet on the ground over a time while the gait is being changed from one to another at a given rate (an infinite rate is an abrupt change), for a given duty.
//
// A walk changed into a trot at the worst moment kept at least 2 feet on the ground when the offsets were switched at once and 2 at a rate of 0.2 cycles per second, so the count of feet does not show the danger; the abrupt change jumped a leg's phase by 0.250 of a cycle in one tick (the commanded foot jumps along its path with it), where the bounded rate moved it 0.0002 a tick, taking 1.25 s in all. Speeds of 0.3 and 0.7 m/s changed the gait down and up, and a speed hovering at 0.5 m/s changed it 0 times with the gap and 175 without.

#ifndef RC_RC_LEGGED_GAIT_SCHEDULE_HPP
#define RC_RC_LEGGED_GAIT_SCHEDULE_HPP

#include <cmath>
#include <rc/legged/stance_swing.hpp>

namespace rc {
namespace legged {

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


}  // namespace legged
}  // namespace rc

#endif
