#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cmath>

struct JointMap {
  double leader_zero = 0.0;
  double follower_zero = 0.0;
  double scale = 1.0;
  double lo = -3.0;
  double hi = 3.0;
};

// The follower's target for a leader reading.
inline double map_joint(const JointMap& m, double leader_q) {
  double t = m.scale * (leader_q - m.leader_zero) + m.follower_zero;
  if (t < m.lo) t = m.lo;
  if (t > m.hi) t = m.hi;
  return t;
}

// One tick of a follower joint toward its target: a lag of time constant tau, at no more than vmax.
inline double follower_step(double q, double target, double tau, double vmax, double dt) {
  double v = (target - q) / tau;
  if (v > vmax) v = vmax;
  if (v < -vmax) v = -vmax;
  return q + v * dt;
}

// Whether a message has come within `timeout` seconds of now.
inline bool link_alive(double now, double last_rx, double timeout) { return now - last_rx <= timeout; }


#endif
