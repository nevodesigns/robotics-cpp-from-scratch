#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <algorithm>
#include <cmath>
#include <limits>

// How far the robot travels between the moment something happens and the moment it is stopped, at a speed (m/s), a deceleration (m/s^2) once the brakes act, and a latency (s) before they do.
// During the latency it goes on at the speed; then it brakes at a constant deceleration: v t + v^2 / (2 a). The speed's sign does not matter. With no braking at all the distance is infinite.
inline double stopping_distance(double speed, double decel, double latency) {
  // TODO: v = |speed|. Zero if v is zero; infinity if decel is not positive; otherwise v * latency (latency not below 0) + v * v / (2 * decel)
  (void)speed;
  (void)decel;
  (void)latency;
  return 0.0;
}

// The largest speed from which the robot can stop inside a sight line (m) with a margin (m) to spare: the v for which stopping_distance(v) = sight - margin.
// From v t + v^2/(2a) = d: v = -a t + sqrt(a^2 t^2 + 2 a d). Zero if nothing is left after the margin, or the robot cannot brake.
inline double max_speed(double sight, double decel, double latency, double margin) {
  // TODO: d = sight - margin. Zero if d <= 0 or decel <= 0. Otherwise with at = decel * latency (latency not below 0): -at + sqrt(at*at + 2*decel*d)
  (void)sight;
  (void)decel;
  (void)latency;
  (void)margin;
  return 0.0;
}

// The deceleration the robot really gets: the brakes can give brake_decel, the ground can give at most friction * g * cos(slope), and the smaller wins; then gravity takes g sin(slope) away on a downhill
// slope (slope in radians, positive downhill, negative uphill, where it adds). Never below zero: a robot that cannot stop has no deceleration, not a negative one.
inline double effective_decel(double brake_decel, double friction, double slope) {
  // TODO: grip = friction * 9.81 * cos(slope); a = min(brake_decel, grip) - 9.81 * sin(slope); never below zero
  (void)brake_decel;
  (void)friction;
  (void)slope;
  return 0.0;
}

// The latency from the event to the brakes acting, in the worst case: the sensor's delay, the stop path's timeout if the event is a lost link, a whole control tick (the event can happen just after one), and the brake's own delay.
inline double total_latency(double sensor_delay, double timeout, double loop_period, double actuator_delay) {
  // TODO: the sum of the four
  (void)sensor_delay;
  (void)timeout;
  (void)loop_period;
  (void)actuator_delay;
  return 0.0;
}

#endif  // LESSON_SOLUTION_HPP
