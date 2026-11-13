#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cmath>
#include <rc/legged/fall_safety.hpp>

using namespace rc::legged;

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
  (void)in;  // TODO: ask for off on the emergency stop, on a fall (kFallen), on a link silent for over link_off, and on heat of at least heat_off; for soft on kFallSoft and a link silent for over link_soft; for derating, with a scale of (heat_off - heat) / (heat_off - heat_warn), when the heat is over heat_warn; the mode is the highest asked, and off while standing becomes soft; the scale is 0 when off
  (void)l;
  return out;
}

inline double apply_scale(double cmd, const SafetyOut& s) { return cmd * s.scale; }


#endif
