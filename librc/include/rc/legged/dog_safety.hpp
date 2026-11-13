// rc/legged/dog_safety.hpp
//
// The layer that can take the power away, from lesson 36-06.
//
// limit_torque clips a joint's torque command to the peak the motor may give. SafetyIn is what the layer looks at: the emergency stop, how long since the last message on the link, the knees' temperature as a fraction of their limit, the fall decision of 35-09, and whether the robot is standing. safety_decide gives the mode (0 normal, 1 derated, 2 soft, 3 off) and the scale to put on the torques: the emergency stop and a fall (fallen) ask for off, a link silent for over the long timeout asks for off and for over the short one asks for soft, a fall that cannot be stopped asks for soft, a temperature over 0.8 asks for derating by a scale falling from one at 0.8 to nothing at 1.0 and over 1.0 for off; the highest of what is asked is the mode; and off while standing is turned into soft. apply_scale puts the scale on a command.
//
// A torque command of 40 N m was cut to 20. Knees at 0.9 of their limit were derated to 0.50 of the torque, at 0.95 to 0.25 and at 1.0 switched off. A link silent for 0.3 s gave 2 and for 2.0 s 3 (0 normal, 1 derated, 2 soft, 3 off); the emergency stop asked for off and, with the robot standing, got 2, and with it sitting 3; a fall that could not be stopped gave 2. With the knees hot (derating) and the link silent for 0.3 s the layer chose 2.

#ifndef RC_RC_LEGGED_DOG_SAFETY_HPP
#define RC_RC_LEGGED_DOG_SAFETY_HPP

#include <cmath>
#include <rc/legged/fall_safety.hpp>

namespace rc {
namespace legged {

enum SafeMode { kSafeNormal = 0, kSafeDerated = 1, kSafeSoft = 2, kSafeOff = 3 };

struct SafetyIn {
  bool estop = false;
  double link_age = 0.0;      // s since the last message
  double heat = 0.0;          // the knees' temperature rise over its limit
  int fall = kFallNone;       // from 35-09
  bool standing = false;
};

struct SafetyLimits {
  double link_soft = 0.1;     // s of silence that asks for soft
  double link_off = 1.0;      // s of silence that asks for off
  double heat_warn = 0.8;     // derating starts here
  double heat_off = 1.0;      // and is complete here; beyond it, off
};

struct SafetyOut {
  int mode = kSafeNormal;
  double scale = 1.0;
};

inline double limit_torque(double cmd, double tau_peak) { return cmd > tau_peak ? tau_peak : (cmd < -tau_peak ? -tau_peak : cmd); }

// The mode and the torque scale: the highest of what is asked, with off while standing turned into soft.
inline SafetyOut safety_decide(const SafetyIn& in, const SafetyLimits& l) {
  SafetyOut out;
  int mode = kSafeNormal;
  double scale = 1.0;
  if (in.estop) mode = kSafeOff;
  if (in.fall == kFallen) mode = kSafeOff;
  if (in.fall == kFallSoft && mode < kSafeSoft) mode = kSafeSoft;
  if (in.link_age > l.link_off) mode = kSafeOff;
  if (in.link_age > l.link_soft && mode < kSafeSoft) mode = kSafeSoft;
  if (in.heat >= l.heat_off) {
    mode = kSafeOff;
  } else if (in.heat > l.heat_warn) {
    scale = (l.heat_off - in.heat) / (l.heat_off - l.heat_warn);
    if (mode < kSafeDerated) mode = kSafeDerated;
  }
  if (mode == kSafeOff && in.standing) mode = kSafeSoft;
  out.mode = mode;
  out.scale = mode == kSafeOff ? 0.0 : scale;
  return out;
}

inline double apply_scale(double cmd, const SafetyOut& s) { return cmd * s.scale; }


}  // namespace legged
}  // namespace rc

#endif
