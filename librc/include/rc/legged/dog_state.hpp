// rc/legged/dog_state.hpp
//
// Sit, stand, walk, fall, from lesson 36-02.
//
// Dog is the state, the time in it, the tries at getting up and the fault code (1 the link was lost, 2 the knees were too hot, 4 a move took too long, 5 the tries at getting up ran out). DogIn is what is sensed and asked: what the operator wants (sit, stand or walk), the speed commanded, whether the battery is enough, the temperature of the knees as a fraction of their limit, whether the link is alive, the fall decision of 35-09, whether the legs have reached the stand or the sit, whether the walk has slowed to a stop, and a reset. dog_step gives the next state: from sitting it rises only if the operator wants more, the battery is enough and the knees are cool (under 0.8); it stands when the legs say so; it walks when asked and the command is not zero, and goes back to standing only once the walk has slowed; it lowers to sit when asked (or at once if the battery is not enough); a fall puts it in fallen, from where it tries to rise again if it is safe and gives up after a number of tries; and the link lost, the knees too hot or a move that takes too long put it in a fault that only a reset clears.
//
// A day of sit, stand, walk, stand and sit went through the states in order (6 changes), rising in 0.9 s. A robot with knees at 0.85 of their limit 1 (1 means it refused to rise); one whose battery was not enough 1. A link lost while walking gave fault 1, knees at their limit fault 2, a rise that never finished fault 4, a fall was followed by 3 tries at getting up and then fault 5, and a robot told to sit while walking at 0.6 m/s first slowed to a stop and then lowered.

#ifndef RC_RC_LEGGED_DOG_STATE_HPP
#define RC_RC_LEGGED_DOG_STATE_HPP

#include <cmath>
#include <rc/legged/fall_safety.hpp>

namespace rc {
namespace legged {

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


}  // namespace legged
}  // namespace rc

#endif
