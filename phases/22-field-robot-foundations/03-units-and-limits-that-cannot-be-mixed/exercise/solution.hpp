#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <algorithm>
#include <cmath>

// A speed that knows its unit. The only way in is a named constructor, so a bare number is never read in the wrong one.
struct Speed {
  double mps = 0.0;
  static Speed from_mps(double v) {
    Speed s;
    s.mps = v;
    return s;
  }
  static Speed from_kmh(double kmh) {
    Speed s;
    s.mps = kmh / 3.6;
    return s;
  }
  double kmh() const { return mps * 3.6; }
};

// A turn rate that knows its unit.
struct Rate {
  double rps = 0.0;
  static Rate from_rps(double w) {
    Rate r;
    r.rps = w;
    return r;
  }
  static Rate from_dps(double degrees_per_second) {
    Rate r;
    r.rps = degrees_per_second * 3.14159265358979323846 / 180.0;
    return r;
  }
};

// What a vehicle may do. Braking is allowed to be stronger than accelerating, which is true of nearly every vehicle.
struct VehicleLimits {
  Speed max_v = Speed::from_mps(1.0);
  Rate max_w = Rate::from_rps(2.0);
  double max_accel = 0.5;    // m/s2, when speeding up
  double max_decel = 1.5;    // m/s2, when slowing down
  double max_alpha = 4.0;    // rad/s2, how fast the turn rate may change
  double max_lateral = 1.5;  // m/s2, speed times turn rate
};

// What may be sent, and which limits changed it.
struct Shaped {
  double v = 0.0;
  double w = 0.0;
  bool limited_speed = false;
  bool limited_rate = false;
  bool limited_lateral = false;
};

inline double clamp_to(double value, double low, double high) { return std::max(low, std::min(high, value)); }

// Applies every limit to a wanted command, given what the vehicle did the step before and the time step.
inline Shaped shape(double want_v, double want_w, double prev_v, double prev_w, double dt, const VehicleLimits& lim) {
  Shaped out;
  (void)want_v;  // TODO: clamp to the speed limits, then limit the change from prev_v and prev_w by the rates, then cut v so that v times w stays within max_lateral
  (void)want_w;
  (void)prev_v;
  (void)prev_w;
  (void)dt;
  (void)lim;
  return out;
}


#endif
