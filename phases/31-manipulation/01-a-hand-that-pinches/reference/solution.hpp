#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cmath>
#include <cstddef>

struct Gripper {
  double stroke = 0.085;     // m, fully open
  double f_min = 20.0;       // N, the squeeze at its weakest setting (typical)
  double f_max = 235.0;      // N, the squeeze at its strongest (typical)
  double speed = 0.15;       // m/s, how fast the fingers close (typical)
  double mu = 0.5;           // friction between pad and thing (typical, rubber on a dry box)
};

constexpr double kGripGravity = 9.81;

// The squeeze that holds `mass` kg while the hand accelerates by `accel` m/s2 against gravity (a lift), with a safety factor; two pads, each giving mu times the squeeze.
inline double required_force(const Gripper& g, double mass, double accel, double safety) {
  return safety * mass * (kGripGravity + accel) / (2.0 * g.mu);
}

// The heaviest mass a squeeze of `force` holds at that acceleration and safety factor.
inline double max_payload(const Gripper& g, double force, double accel, double safety) {
  return 2.0 * g.mu * force / (safety * (kGripGravity + accel));
}

// Whether a width lies between the fingers when they are open.
inline bool fits(const Gripper& g, double width) { return width >= 0.0 && width <= g.stroke; }

// Seconds for the fingers to close from open onto a width; a negative number if it does not fit.
inline double close_time(const Gripper& g, double width) {
  if (!fits(g, width)) return -1.0;
  return (g.stroke - width) / g.speed;
}

// The squeeze over what holding needs (1 is exactly enough, and with a safety factor in `needed` the margin is over that).
inline double grip_margin(double force, double needed) {
  if (needed <= 0.0) return 1e9;
  return force / needed;
}


#endif
