#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cmath>

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
  (void)v;  // TODO: the hip's place plus half the distance the body goes in the stance (v times the stance time over two) plus k times (v minus v_cmd)
  (void)v_cmd;
  (void)t_stance;
  (void)k;
  for (int i = 0; i < 2; ++i) out[i] = hip[i];
}


#endif
