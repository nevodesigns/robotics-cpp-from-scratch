// rc/legged/terrain_adapt.hpp
//
// Feet that find the ground, from lesson 36-04.
//
// ReachState is how far below its planned place a foot has been put down while it searches (ext), whether it has found the ground (done) and whether it gave up (hole). reach_down_step moves the search on by one tick: while the contact sense (34-07) says the foot is not down it lowers the foot by rate times dt, and it stops as soon as the foot is down, which is done; if the foot has gone as far below its planned place as the leg may (max_extra) without touching, it stops and says hole. update_ground moves an estimate of the ground's height under a foot toward the height at which it touched by a gain, so that a single bump moves the estimate only a little; body_height_from gives the height the body should be at: the nominal height plus the mean of the estimates of the feet that are down.
//
// On level ground the swing ends 1 cm above the ground and the search took 0.010 m of extension and 0.04 s to close it; a bump of 3 cm was met before the swing ended and needed 0.000 m; a step down of 5 cm took an extension of 0.060 m and 0.24 s at 0.25 m/s, and a hole 15 cm deep, deeper than the 10 cm the leg may reach, was reported as a hole after 0.40 s. A gain of 0.2 moved the estimate 6.0 mm after one landing on a 3 cm bump and 26.8 mm after ten; the body followed the mean of four feet by 15 mm.

#ifndef RC_RC_LEGGED_TERRAIN_ADAPT_HPP
#define RC_RC_LEGGED_TERRAIN_ADAPT_HPP

#include <cmath>

namespace rc {
namespace legged {

struct ReachState {
  double ext = 0.0;      // m below the planned place that the foot has been put down
  bool done = false;     // the ground has been found
  bool hole = false;     // the foot went as far as the leg may and found nothing
};

// One tick of the search: contact is the sense of 34-07.
inline void reach_down_step(ReachState& s, bool contact, double dt, double rate, double max_extra) {
  if (s.done || s.hole) return;
  if (contact) {
    s.done = true;
    return;
  }
  s.ext += rate * dt;
  if (s.ext >= max_extra) {
    s.ext = max_extra;
    s.hole = true;
  }
}

// Moves an estimate of the ground's height under a foot toward the height at which it touched, by a gain between 0 and 1.
inline double update_ground(double estimate, double touched, double gain) { return estimate + gain * (touched - estimate); }

// The height the body should be at: the nominal height plus the mean of the ground estimates of the feet that are down.
inline double body_height_from(double nominal, const double est[4], const bool down[4]) {
  double sum = 0.0;
  int n = 0;
  for (int i = 0; i < 4; ++i)
    if (down[i]) {
      sum += est[i];
      ++n;
    }
  return nominal + (n > 0 ? sum / n : 0.0);
}


}  // namespace legged
}  // namespace rc

#endif
