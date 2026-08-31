#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cmath>

struct Pose {
  double x = 0.0;
  double y = 0.0;
  double theta = 0.0;
};

inline double wrap_angle(double radians) {
  return std::atan2(std::sin(radians), std::cos(radians));
}

inline Pose step(const Pose& start, double vl, double vr, double wheel_base, double dt) {
  // Forward speed is the average of the two wheels. Equal and opposite speeds
  // average to zero, which is why a spin in place does not change x or y.
  const double v = (vl + vr) / 2.0;

  // Turn rate comes from the difference. A wider wheel base turns more slowly
  // for the same difference, which is why the base is divided rather than
  // multiplied.
  const double omega = (vr - vl) / wheel_base;

  Pose next;
  // Cosine gives the x part of a direction, sine gives the y part. Swapping
  // these two lines is the single most common mistake in this lesson.
  next.x = start.x + v * dt * std::cos(start.theta);
  next.y = start.y + v * dt * std::sin(start.theta);

  // Wrapping keeps theta comparable. Without it the angle grows without limit
  // and every later heading calculation quietly breaks.
  next.theta = wrap_angle(start.theta + omega * dt);
  return next;
}

#endif  // LESSON_SOLUTION_HPP
