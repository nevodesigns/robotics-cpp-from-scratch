// rc/legged/stance_swing.hpp
//
// Down and up, from lesson 34-04.
//
// GaitTiming holds the period of the gait, the duty factor (the fraction of the period that a foot is on the ground) and a phase offset for each of the four legs. leg_phase is the fraction of the period a leg has done at a time, from zero to one: the leg is in stance while the phase is below the duty and in swing after it. count_stance is how many of the four feet are on the ground at a time. stance_foot_velocity gives the velocity of a foot that is on the ground in the body's frame: minus the body's velocity and minus the body's turn about the foot's place. touchdown_xy is Raibert's rule for where to put a swinging foot down: the default place under the hip, moved forward by half the distance the body will travel while the foot is on the ground, so that the foot leaves as far behind the hip as it arrives ahead of it, and by k times the amount by which the body is going faster than commanded, which puts the foot farther ahead and so slows the body.
//
// A walk with a duty of 0.75 always had at least 3 feet down and a trot of 0.5 had 2; with a duty of 0.35 30 percent of the period had no foot down. At 0.4 m/s with a stance of 0.25 s the foot was put down 0.050 m ahead of its default place and left -0.050 m behind it. With the body 0.2 m/s too fast and a gain of 0.1 the foot went a further 20 mm ahead.

#ifndef RC_RC_LEGGED_STANCE_SWING_HPP
#define RC_RC_LEGGED_STANCE_SWING_HPP

#include <cmath>

namespace rc {
namespace legged {

struct GaitTiming {
  double period = 0.5;       // s
  double duty = 0.5;         // the fraction of the period each foot is on the ground
  double offset[4] = {0.0, 0.5, 0.5, 0.0};  // the phase offset of each leg (a trot: the diagonal pairs together)
};

// The phase of a leg at a time: the fraction of the period it has done, from 0 up to but not including 1.
inline double leg_phase(const GaitTiming& g, int leg, double t) {
  double p = t / g.period + g.offset[leg];
  p -= std::floor(p);
  return p;
}

inline bool in_stance(const GaitTiming& g, int leg, double t) { return leg_phase(g, leg, t) < g.duty; }

// How many of the four feet are on the ground at a time.
inline int count_stance(const GaitTiming& g, double t) {
  int n = 0;
  for (int i = 0; i < 4; ++i)
    if (in_stance(g, i, t)) ++n;
  return n;
}

// The velocity of a foot that is on the ground, in the body's frame: minus the body's velocity and minus its turn about the foot's place.
inline void stance_foot_velocity(const double v_body[2], double yaw_rate, const double foot[2], double out[2]) {
  out[0] = -v_body[0] + yaw_rate * foot[1];
  out[1] = -v_body[1] - yaw_rate * foot[0];
}

// Where to put a swinging foot down (x, y in the body's frame) for a hip at `hip`, a body velocity v, a commanded velocity v_cmd, a stance time and a gain k.
inline void touchdown_xy(const double hip[2], const double v[2], const double v_cmd[2], double t_stance, double k, double out[2]) {
  for (int i = 0; i < 2; ++i) out[i] = hip[i] + 0.5 * v[i] * t_stance + k * (v[i] - v_cmd[i]);
}


}  // namespace legged
}  // namespace rc

#endif
