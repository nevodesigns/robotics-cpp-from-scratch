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
  (void)force;  // TODO: while off, count readings over f_on (but none inside the blank after lift) and go on after `need` in a row; while on, count readings under f_off and go off after `need` in a row; a reading on the wrong side resets the run
  (void)since_lift;
  (void)p;
  return s.down;
}


#endif
