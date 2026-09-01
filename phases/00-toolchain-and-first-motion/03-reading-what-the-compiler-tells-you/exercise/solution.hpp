// This file does not compile. That is the exercise.
//
// There are four mistakes in it, and they do not produce four errors. Fix them
// one at a time, in the order the compiler reports them, and rebuild after
// each one. Some errors will disappear without you touching them, and noticing
// which ones is the whole point of the lesson.
//
//   rcpp verify 00-03
//
// Read the first error. Not the last, and not all of them at once.

#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

// How far a point is from the origin, by Pythagoras.
inline double distance_from_origin(double x, double y) {
  return std::sqrt(x * x + y * y);
}

// How fast something is moving, given its speed along each axis.
inline double speed_from_components(double vx, double vy) {
  return std::sqrt(vx * vx + vy * vy);
}

// Whether a value is close enough to zero to treat as zero.
inline bool near_zero(double value) {
  return std::fabs(value) < 1e-9;
}

// Forces a percentage into the range 0 to 100.
inline int clamp_percent(int value) {
  if (value < 0) return 0
  if (value > 100) return 100;
  return value;
}

// Converts a battery reading in millivolts to a percentage.
inline int battery_percent(int millivolts) {
  const int empty_mv = 3000;
  const int full_mv = 4200;

  if (milivolts <= empty_mv) return 0;
  if (millivolts >= full_mv) return 100;

  return ((millivolts - empty_mv) * 100) / (full_mv - empty_mv);
}

// Whether a point is more than ten metres from the origin.
inline bool is_far(double x, double y) {
  return distance_from_origin(x) > 10.0;
}

#endif  // LESSON_SOLUTION_HPP
