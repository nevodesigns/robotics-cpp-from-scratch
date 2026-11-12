#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <algorithm>
#include <cmath>
#include <rc/safety/stopping.hpp>

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
  // TODO: Zero if the range or age is NaN, if the age is above max_age or below 0, or if the range is below min_valid.
  // If the range is infinite or above max_valid: min(no_return_range, max_valid). Otherwise the range.
  (void)r;
  (void)c;
  return 0.0;
}

// The speed to send: the command, cut down to the top speed and to what the range in the direction of travel allows (front for a forward command, rear for a reverse one). The sign of the command is kept; zero stays zero.
inline double limit_speed(double command, const Reading& front, const Reading& rear, const LimiterConfig& c) {
  // TODO: Zero if the command is NaN or zero. The range is trusted_range of the front for a positive command, of the rear for a negative one.
  // The speed is min(|command|, top_speed, max_speed(range, decel, latency, margin)); give it the sign of the command
  (void)command;
  (void)front;
  (void)rear;
  (void)c;
  return 0.0;
}

#endif  // LESSON_SOLUTION_HPP
