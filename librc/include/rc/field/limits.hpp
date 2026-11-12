// rc/field/limits.hpp
//
// Units and limits for field vehicles, from lesson 22-03.
//
// Speed and Rate keep the unit in the type (from_kmh, from_mps, from_dps, from_rps), so a limit cannot be read in the wrong one. VehicleLimits holds the speed, turn rate, acceleration, braking, turn acceleration and lateral acceleration of a vehicle. shape() takes what a planner wants and what the vehicle is doing and returns what may be sent: clamped, rate limited, and slowed so that speed times turn rate stays within the lateral limit.
//
// 5 km/h is 1.389 m/s; a ramp to 1 m/s at 0.5 m/s2 took 2.000 s; a stop from 1 m/s at 1.5 m/s2 took 0.667 s and 0.333 m; 1 m/s at 2 rad/s was cut to 0.750 m/s at a lateral limit of 1.5 m/s2; a limit of 5 read as m/s instead of km/h made the stopping distance 8.331 m instead of 0.642 m.

#ifndef RC_RC_FIELD_LIMITS_HPP
#define RC_RC_FIELD_LIMITS_HPP

#include <algorithm>
#include <cmath>

namespace rc {
namespace field {

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
  double v = clamp_to(want_v, -lim.max_v.mps, lim.max_v.mps);
  double w = clamp_to(want_w, -lim.max_w.rps, lim.max_w.rps);
  if (v != want_v || w != want_w) out.limited_speed = true;

  const double allowed = (std::fabs(v) > std::fabs(prev_v) ? lim.max_accel : lim.max_decel) * dt;
  const double dv = clamp_to(v - prev_v, -allowed, allowed);
  if (dv != v - prev_v) out.limited_rate = true;
  v = prev_v + dv;

  const double turn_allowed = lim.max_alpha * dt;
  const double dw = clamp_to(w - prev_w, -turn_allowed, turn_allowed);
  if (dw != w - prev_w) out.limited_rate = true;
  w = prev_w + dw;

  if (std::fabs(v * w) > lim.max_lateral) {
    v = std::copysign(lim.max_lateral / std::fabs(w), v);
    out.limited_lateral = true;
  }
  out.v = v;
  out.w = w;
  return out;
}


}  // namespace field
}  // namespace rc

#endif
