#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cmath>

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


#endif
