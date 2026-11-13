#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cmath>
#include <rc/arm/fk.hpp>
#include <rc/arm/frames.hpp>
#include <rc/arm/ik_closed.hpp>
#include <vector>

using namespace rc::arm;

using rc::arm::apply;
using rc::arm::ArmTable;
using rc::arm::compose;
using rc::arm::forward;
using rc::arm::inverse;
using rc::arm::Pose;
using rc::arm::rot_z;
using rc::arm::trans;

struct BasePose {
  double x = 0.0;
  double y = 0.0;
  double heading = 0.0;  // rad, about the vertical, from the world x axis
};

struct MobileManipulator {
  ArmTable arm;
  Pose mount;   // the arm's base frame in the base frame
  Pose tool;    // the tool in the arm's flange frame
};

inline Pose base_pose(const BasePose& b) { return compose(trans(b.x, b.y, 0.0), rot_z(b.heading)); }

inline Pose arm_base_in_world(const MobileManipulator& m, const BasePose& b) { return compose(base_pose(b), m.mount); }

// The tool's pose in the world.
inline Pose tool_in_world(const MobileManipulator& m, const BasePose& b, const std::vector<double>& q) {
  return forward(m.arm, q, arm_base_in_world(m, b), m.tool);
}

// A point given in the world, in the arm's own base frame.
inline void world_to_arm(const MobileManipulator& m, const BasePose& b, const double world[3], double out[3]) {
  apply(inverse(arm_base_in_world(m, b)), world, out);
}

// How far the hand moves for an error of `dpos` metres in the base's place and `dheading` radians in its heading: the position error one for one, the heading error times the ground distance from the base's centre to the hand.
inline double hand_error_from_base(const BasePose& b, const double hand[3], double dpos, double dheading) {
  const double lever = std::hypot(hand[0] - b.x, hand[1] - b.y);
  return std::hypot(dpos, dheading * lever);
}


#endif
