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
  if (d.s == kDFault) {
    if (in.reset) {
      d.fault = 0;
      d.tries = 0;
      dog_enter(d, kDSit);
    }
    return;
  }
  const bool up = d.s == kDRising || d.s == kDStand || d.s == kDWalk || d.s == kDLowering;
  if (up && !in.link_ok) {
    d.fault = 1;
    dog_enter(d, kDFault);
    return;
  }
  if (in.heat >= 1.0 && d.s != kDSit) {
    d.fault = 2;
    dog_enter(d, kDFault);
    return;
  }
  if (up && in.fall == kFallen) {
    dog_enter(d, kDFallen);
    return;
  }
  if ((d.s == kDRising || d.s == kDLowering) && d.t > d.move_timeout) {
    d.fault = 4;
    dog_enter(d, kDFault);
    return;
  }
  const bool safe_to_rise = in.battery_ok && in.heat < 0.8 && in.link_ok;
  switch (d.s) {
    case kDSit:
      if (in.want >= kWantStand && safe_to_rise) dog_enter(d, kDRising);
      break;
    case kDRising:
      if (in.stood) dog_enter(d, kDStand);
      break;
    case kDStand:
      if (in.want == kWantSit || !in.battery_ok) {
        dog_enter(d, kDLowering);
      } else if (in.want == kWantWalk && std::fabs(in.cmd_speed) > 0.02) {
        dog_enter(d, kDWalk);
      }
      break;
    case kDWalk:
      if ((in.want != kWantWalk || std::fabs(in.cmd_speed) <= 0.02 || !in.battery_ok) && in.stopped) dog_enter(d, kDStand);
      break;
    case kDLowering:
      if (in.sat) dog_enter(d, kDSit);
      break;
    case kDFallen:
      if (in.want >= kWantStand && safe_to_rise) {
        if (d.tries >= d.max_tries) {
          d.fault = 5;
          dog_enter(d, kDFault);
        } else {
          ++d.tries;
          dog_enter(d, kDRising);
        }
      }
      break;
    case kDFault:
      break;
  }
  if (d.s == kDStand && in.stood) d.tries = 0;
}


#endif
