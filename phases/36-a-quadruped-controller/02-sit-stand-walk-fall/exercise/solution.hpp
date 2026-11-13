#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cmath>
#include <rc/legged/fall_safety.hpp>

using namespace rc::legged;

enum DogState { kDSit, kDRising, kDStand, kDWalk, kDLowering, kDFallen, kDFault };
enum Want { kWantSit = 0, kWantStand = 1, kWantWalk = 2 };

struct Dog {
  DogState s = kDSit;
  double t = 0.0;      // s in this state
  int tries = 0;       // tries at getting up after a fall
  int fault = 0;       // 1 link, 2 hot, 4 a move took too long, 5 out of tries
  int max_tries = 3;
  double move_timeout = 4.0;
};

struct DogIn {
  int want = kWantSit;
  double cmd_speed = 0.0;
  bool battery_ok = true;
  double heat = 0.0;       // the knees' temperature rise over its limit
  bool link_ok = true;
  int fall = kFallNone;    // from 35-09
  bool stood = false;      // the legs have reached the stand
  bool sat = true;         // the legs have reached the sit
  bool stopped = true;     // the walk has slowed to a stop
  bool reset = false;
};

inline void dog_enter(Dog& d, DogState s) {
  d.s = s;
  d.t = 0.0;
}

// One tick: the next state.
inline void dog_step(Dog& d, const DogIn& in, double dt) {
  d.t += dt;
  (void)in;  // TODO: a fault stays until in.reset (then sit); while up, the link lost is fault 1, the heat at or over 1 is fault 2 (not when sitting), a fall (35-09's kFallen) puts it in fallen, rising or lowering for over move_timeout is fault 4; from sit, rise if the operator wants stand or walk and the battery is ok and the heat under 0.8; rising to stand when stood; stand to lowering if sit is wanted or the battery is not ok, to walk if walk is wanted and the command is not zero; walk back to stand only when stopped; lowering to sit when sat; fallen tries to rise again if wanted and safe, counting the tries and fault 5 after max_tries
}


#endif
