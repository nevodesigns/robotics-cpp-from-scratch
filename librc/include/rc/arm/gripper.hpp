// rc/arm/gripper.hpp
//
// A hand that pinches, from lesson 31-01.
//
// Gripper is the numbers of a parallel two finger hand: stroke (metres from closed to fully open), the least and most force it can squeeze with (newtons, for the two fingers together as a squeeze on the thing), the speed the fingers close at and the coefficient of friction between the pads and the thing. required_force gives the squeeze that holds a mass against gravity and an acceleration of the hand, with a safety factor, for the friction at the two pads. max_payload is the same turned round. fits says whether a width lies between the fingers. close_time is how long the fingers take to close on a width. grip_margin is the force over what is needed.
//
// A hand with a stroke of 0.085 m and a squeeze of up to 235 N held 1 kg at rest with 9.81 N at a friction of 0.5; lifting it at 5 m/s2 took 14.81 N. The same hand held at most 16.0 kg with dry pads and 8.0 kg with pads at half the friction.

#ifndef RC_RC_ARM_GRIPPER_HPP
#define RC_RC_ARM_GRIPPER_HPP

#include <cmath>
#include <cstddef>

namespace rc {
namespace arm {

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


}  // namespace arm
}  // namespace rc

#endif
