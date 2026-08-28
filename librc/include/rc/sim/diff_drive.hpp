// rc/sim/diff_drive.hpp
//
// The differential drive model, graduated from lesson 00-04.
//
// This is the first artifact in the curriculum that a later phase depends on.
// The learner writes it in phase 00 as their first piece of real robotics
// arithmetic, and phase 10 puts a Qt window around this exact function to watch
// the path draw itself.
//
// Kept deliberately small: two wheel speeds, a wheel base and a time step in,
// one pose out. Everything else that a robot needs is built on top rather than
// added here.

#ifndef RC_SIM_DIFF_DRIVE_HPP
#define RC_SIM_DIFF_DRIVE_HPP

#include <cmath>
#include <vector>

namespace rc {
namespace sim {

// Where the robot is and which way it faces. Angles are radians throughout,
// because every trigonometry function in the standard library expects them.
struct Pose {
  double x = 0.0;
  double y = 0.0;
  double theta = 0.0;
};

// Brings an angle into the range minus pi to pi, so two headings can be
// meaningfully compared. Without this the value grows without limit.
inline double wrap_angle(double radians) {
  return std::atan2(std::sin(radians), std::cos(radians));
}

// Advances the robot by one time step.
//
// Forward speed is the average of the wheel speeds, turn rate is their
// difference over the wheel base. The step is treated as a straight line, which
// is an approximation whose error shrinks with dt and which is what real
// odometry uses.
inline Pose step(const Pose& start, double left_speed, double right_speed,
                 double wheel_base, double dt) {
  const double forward = (left_speed + right_speed) / 2.0;
  const double turn = (right_speed - left_speed) / wheel_base;

  Pose next;
  next.x = start.x + forward * dt * std::cos(start.theta);
  next.y = start.y + forward * dt * std::sin(start.theta);
  next.theta = wrap_angle(start.theta + turn * dt);
  return next;
}

// Convenience for anything that wants a whole path rather than one step, which
// is most callers that are about to draw something.
inline std::vector<Pose> drive(const Pose& start, double left_speed, double right_speed,
                               double wheel_base, double dt, int steps) {
  std::vector<Pose> path;
  path.reserve(static_cast<std::size_t>(steps > 0 ? steps : 0));

  Pose pose = start;
  for (int i = 0; i < steps; ++i) {
    pose = step(pose, left_speed, right_speed, wheel_base, dt);
    path.push_back(pose);
  }
  return path;
}

}  // namespace sim
}  // namespace rc

#endif  // RC_SIM_DIFF_DRIVE_HPP
