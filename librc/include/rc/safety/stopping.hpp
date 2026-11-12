// rc/safety/stopping.hpp
//
// Stopping distance and the speed a sight line allows, from lesson 19-04, graduated.
//
// stopping_distance is v t + v^2/(2a): the robot goes on during the latency and then brakes. max_speed inverts it for a sight line and a margin. effective_decel takes the smaller of what the brakes and the ground (friction times g times cos of the slope) can give and takes gravity's pull on a downhill slope off it, never below zero. total_latency adds the sensor, the stop path's timeout, a whole tick and the brake.
//
// The formula matched a robot stepped to a stop within 0.005 m from 0.5 to 5 m/s (2 m/s took 1.600 m at 2 m/s^2 and 0.3 s). A 3 m sight line allowed 3.347 m/s with no latency and 1.899 m/s (57 percent) with a second. On a dry flat floor the deceleration was 3.00, on a wet 10 degree downhill 1.19, and on a wet 17 degree downhill 0: it cannot stop. A latency of 0.56 s put 1.12 m before the brakes at 2 m/s.

#ifndef RC_SAFETY_STOPPING_HPP
#define RC_SAFETY_STOPPING_HPP

#include <algorithm>
#include <cmath>
#include <limits>

namespace rc {
namespace safety {

// How far the robot travels between the moment something happens and the moment it is stopped, at a speed (m/s), a deceleration (m/s^2) once the brakes act, and a latency (s) before they do.
// During the latency it goes on at the speed; then it brakes at a constant deceleration: v t + v^2 / (2 a). The speed's sign does not matter. With no braking at all the distance is infinite.
inline double stopping_distance(double speed, double decel, double latency) {
  const double v = std::fabs(speed);
  if (v == 0.0) return 0.0;
  if (decel <= 0.0) return std::numeric_limits<double>::infinity();
  return v * std::max(latency, 0.0) + v * v / (2.0 * decel);
}

// The largest speed from which the robot can stop inside a sight line (m) with a margin (m) to spare: the v for which stopping_distance(v) = sight - margin.
// From v t + v^2/(2a) = d: v = -a t + sqrt(a^2 t^2 + 2 a d). Zero if nothing is left after the margin, or the robot cannot brake.
inline double max_speed(double sight, double decel, double latency, double margin) {
  const double d = sight - margin;
  if (d <= 0.0 || decel <= 0.0) return 0.0;
  const double at = decel * std::max(latency, 0.0);
  return -at + std::sqrt(at * at + 2.0 * decel * d);
}

// The deceleration the robot really gets: the brakes can give brake_decel, the ground can give at most friction * g * cos(slope), and the smaller wins; then gravity takes g sin(slope) away on a downhill
// slope (slope in radians, positive downhill, negative uphill, where it adds). Never below zero: a robot that cannot stop has no deceleration, not a negative one.
inline double effective_decel(double brake_decel, double friction, double slope) {
  const double g = 9.81;
  const double grip = friction * g * std::cos(slope);
  const double a = std::min(brake_decel, grip) - g * std::sin(slope);
  return std::max(a, 0.0);
}

// The latency from the event to the brakes acting, in the worst case: the sensor's delay, the stop path's timeout if the event is a lost link, a whole control tick (the event can happen just after one), and the brake's own delay.
inline double total_latency(double sensor_delay, double timeout, double loop_period, double actuator_delay) {
  return sensor_delay + timeout + loop_period + actuator_delay;
}

}  // namespace safety
}  // namespace rc

#endif  // RC_SAFETY_STOPPING_HPP
