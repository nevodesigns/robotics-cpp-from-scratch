// rc/safety/limiter.hpp
//
// A speed limiter from range, from lesson 19-05, graduated.
//
// trusted_range turns a Reading (range and age) into the clear distance it can vouch for: a reading that is not a number, is older than max_age, is inside the blind zone or is negative is a blocked path (zero), and a reading of nothing (infinite or beyond the sensor's reach) is taken as no_return_range, not as the whole reach, because a dark, glossy or angled surface returns nothing.
// limit_speed cuts a command to the top speed and to rc::safety::max_speed of the range in the direction of travel (front for forward, rear for reverse), keeping the sign.
//
// A range that was not a number gave min(top speed, max_speed(NaN)) = 2.0 m/s through the obvious code and 0 through the limiter. Driving at a wall 10 m away at a command of 2 m/s with a margin of 0.3 m, the limiter stopped the robot at 0.30 m; without it it hit at 2.00 m/s. A reading 0.25 s old allowed 0.

#ifndef RC_SAFETY_LIMITER_HPP
#define RC_SAFETY_LIMITER_HPP

#include <algorithm>
#include <cmath>
#include <rc/safety/stopping.hpp>

namespace rc {
namespace safety {

struct LimiterConfig {
  double decel = 2.0;            // what the robot can really brake at, worst floor (lesson 19-04)
  double latency = 0.5;          // the whole chain to the brake
  double margin = 0.3;           // what is to be left between the robot and the thing
  double top_speed = 2.0;        // never faster than this
  double min_valid = 0.1;        // a reading closer than this is the sensor's blind zone, not a measurement
  double max_valid = 12.0;       // beyond this the sensor says nothing
  double no_return_range = 1.5;  // what a reading of "nothing there" is allowed to be taken as: far less than the sensor's reach
  double max_age = 0.2;          // a reading older than this (s) is not a reading
};

struct Reading {
  double range = 0.0;   // metres; may be NaN, infinite, zero or negative when the sensor has nothing to say
  double age = 0.0;     // seconds since it was taken
};

// The clear distance a reading can be trusted for, in metres. A reading that is not a number, or is older than max_age, or is closer than the blind zone (or negative) says nothing about the path, and what it says nothing about
// is treated as blocked: zero. A reading of "nothing there" (infinite, or beyond max_valid) is not evidence of an empty path either, since a dark or glossy or angled surface returns nothing: it is
// taken as no_return_range and no more. Anything else is the range itself.
inline double trusted_range(const Reading& r, const LimiterConfig& c) {
  if (std::isnan(r.range) || std::isnan(r.age)) return 0.0;
  if (r.age > c.max_age || r.age < 0.0) return 0.0;
  if (r.range < c.min_valid) return 0.0;
  if (std::isinf(r.range) || r.range > c.max_valid) return std::min(c.no_return_range, c.max_valid);
  return r.range;
}

// The speed to send: the command, cut down to the top speed and to what the range in the direction of travel allows (front for a forward command, rear for a reverse one). The sign of the command is kept; zero stays zero.
inline double limit_speed(double command, const Reading& front, const Reading& rear, const LimiterConfig& c) {
  if (std::isnan(command) || command == 0.0) return 0.0;
  const double range = trusted_range(command > 0.0 ? front : rear, c);
  const double allowed = std::min(c.top_speed, rc::safety::max_speed(range, c.decel, c.latency, c.margin));
  const double v = std::min(std::fabs(command), allowed);
  return command > 0.0 ? v : -v;
}

}  // namespace safety
}  // namespace rc

#endif  // RC_SAFETY_LIMITER_HPP
