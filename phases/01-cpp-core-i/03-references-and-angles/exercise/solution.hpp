#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cmath>

struct Pose {
  double x = 0.0;
  double y = 0.0;
  double theta = 0.0;
};

constexpr double kPi = 3.14159265358979323846;

// Brings any angle into the range minus pi to pi.
//
// Write this one with std::fmod rather than trigonometry. Remember that fmod
// keeps the sign of its left operand, so fmod(-7.0, 6.28) is negative.
inline double wrap_angle(double radians) {
  // TODO
  return radians;
}

// The signed shortest rotation from one heading to another.
// Positive means anticlockwise. Never more than pi in either direction.
inline double shortest_turn(double from, double to) {
  // TODO
  (void)from;
  return to;
}

// Rotates pose towards target_heading by at most max_turn radians.
// Modifies pose in place, and leaves its heading wrapped.
inline void steer_towards(Pose& pose, double target_heading, double max_turn) {
  // TODO: find the shortest turn, take at most max_turn of it, then wrap.
  (void)pose;
  (void)target_heading;
  (void)max_turn;
}

#endif  // LESSON_SOLUTION_HPP
