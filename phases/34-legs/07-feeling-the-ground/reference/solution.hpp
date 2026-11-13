#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cmath>

struct ContactParams {
  double f_on = 8.0;       // N, the force that says the foot is on the ground
  double f_off = 4.0;      // N, the force under which it is said to be off
  int need = 3;            // readings in a row before a change is believed
  double blank = 0.06;     // s after the foot is told to lift during which touchdown is not believed
};

struct ContactState {
  bool down = false;
  int run = 0;
};

// One reading: the estimated force at the foot and the time since the foot was told to lift; whether the foot is on the ground.
inline bool contact_update(ContactState& s, double force, double since_lift, const ContactParams& p) {
  if (!s.down) {
    if (since_lift >= p.blank && force > p.f_on) {
      ++s.run;
    } else {
      s.run = 0;
    }
    if (s.run >= p.need) {
      s.down = true;
      s.run = 0;
    }
  } else {
    if (force < p.f_off) {
      ++s.run;
    } else {
      s.run = 0;
    }
    if (s.run >= p.need) {
      s.down = false;
      s.run = 0;
    }
  }
  return s.down;
}


#endif
