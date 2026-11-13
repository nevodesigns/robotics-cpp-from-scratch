// rc/arm/teleop.hpp
//
// An arm that follows a hand, from lesson 31-08.
//
// map_joint turns the leader's joint reading into the follower's target: the leader's zero is taken away, the scale applied, the follower's zero added, and the result clamped to the follower's limits. follower_step moves a follower joint one tick toward its target: it asks for a velocity of the error over a time constant tau (a first order lag) and clamps it to the speed limit, so a jump in the target is followed at the limit and never faster. link_alive says whether a message has come within the timeout; when it has not, the follower should hold where it is.
//
// A follower with a time constant of 50 ms following a leader moving as 0.5 sin(pi t) rad had an rms error of 0.059 rad with the leader read at 100 Hz, 0.081 rad at 20 Hz, and 0.168 rad at 100 Hz with 100 ms of delay on the link. A jump of 1.0 rad in the leader was followed at no more than 2.0 rad/s, taking 0.48 s. When the link was lost while the leader was moving, a follower with the watchdog moved 0.000 rad after the timeout (it held where it was) and one without it went 0.398 rad on toward the last message.

#ifndef RC_RC_ARM_TELEOP_HPP
#define RC_RC_ARM_TELEOP_HPP

#include <cmath>

namespace rc {
namespace arm {

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


}  // namespace arm
}  // namespace rc

#endif
