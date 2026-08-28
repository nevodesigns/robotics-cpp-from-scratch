#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cmath>

struct Pose {
  double x = 0.0;
  double y = 0.0;
  double theta = 0.0;
};

constexpr double kPi = 3.14159265358979323846;

inline double wrap_angle(double radians) {
  const double full_turn = 2.0 * kPi;

  // fmod leaves the result in minus full_turn to plus full_turn, keeping the
  // sign of the input. That is halfway there.
  double wrapped = std::fmod(radians + kPi, full_turn);

  // fmod(-1.0, 6.28) is -1.0, not 5.28, so a negative result needs one more
  // turn added before shifting back. Forgetting this is the whole bug.
  if (wrapped < 0.0) wrapped += full_turn;

  return wrapped - kPi;
}

inline double shortest_turn(double from, double to) {
  // Subtract first, then wrap. Wrapping the difference is what turns 358
  // degrees into minus 2 degrees.
  return wrap_angle(to - from);
}

inline void steer_towards(Pose& pose, double target_heading, double max_turn) {
  const double turn = shortest_turn(pose.theta, target_heading);

  // Take the whole turn when it is small enough, exactly as the rate limiter in
  // lesson 01-02 does, so the heading settles instead of oscillating.
  if (turn > max_turn) {
    pose.theta = wrap_angle(pose.theta + max_turn);
  } else if (turn < -max_turn) {
    pose.theta = wrap_angle(pose.theta - max_turn);
  } else {
    pose.theta = wrap_angle(target_heading);
  }
}

#endif  // LESSON_SOLUTION_HPP
