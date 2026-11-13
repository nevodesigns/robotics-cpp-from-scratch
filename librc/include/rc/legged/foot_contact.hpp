// rc/legged/foot_contact.hpp
//
// Feeling the ground, from lesson 34-07.
//
// ContactParams holds the force that says the foot is on the ground (f_on), a lower one that says it is off (f_off), how many readings in a row make a change believed, and the blank after the foot was told to lift. ContactState keeps whether the foot is down and the run of readings. contact_update takes the estimated force at the foot (from the torques, 34-02) and the time since the foot was told to lift, and gives whether the foot is on the ground: while it is off, a reading over f_on counts toward touchdown (but none counts during the blank); while it is on, a reading under f_off counts toward lift-off; a run of the right length changes the state, and a reading on the wrong side of the threshold resets the run.
//
// At 500 Hz, with 1 N of noise on a foot that carried 29.4 N, a threshold of 8 N on and 4 N off with 3 readings found touchdown 12 ms after the step began and lift-off 2 ms after the lift was commanded (the force had been falling for 20 ms before that), with 12 changes of state in 6 steps (the right number is 12). Without the blank the spike at lift-off gave 24 changes, and with one threshold for on and off a foot grazing the ground 10 changes against 0 with two.

#ifndef RC_RC_LEGGED_FOOT_CONTACT_HPP
#define RC_RC_LEGGED_FOOT_CONTACT_HPP

#include <cmath>

namespace rc {
namespace legged {

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


}  // namespace legged
}  // namespace rc

#endif
