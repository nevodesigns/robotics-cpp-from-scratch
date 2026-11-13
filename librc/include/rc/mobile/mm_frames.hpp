// rc/mobile/mm_frames.hpp
//
// A base and an arm in one frame tree, from lesson 33-01.
//
// BasePose is a base's place on flat ground: x, y and heading. MobileManipulator holds the arm's table, where the arm is mounted on the base (a Pose in the base frame) and the tool. base_pose gives the 4 by 4 pose of the base in the world; arm_base_in_world is that composed with the mount. tool_in_world is the tool's pose in the world for a base pose and the joint angles. world_to_arm puts a point given in the world into the arm's own base frame, which is where the arm's inverse kinematics wants it. hand_error_from_base gives how far the hand is moved by an error in the base's position and an error in its heading: the position error carries over one for one, and the heading error is multiplied by the distance from the base's centre to the hand, measured on the ground.
//
// With the base at (2.0, 1.0) turned 90 degrees and the arm at its zero pose, the tool was at (2.000, 1.550, 0.050) in the world; a point in the world put into the arm's frame and back gave it to 0.000000000 m. An error of 5 mm in the base's place moved the hand 5 mm, and an error of 1 degree in its heading moved the hand 15.7 mm with the hand 0.9 m from the centre of the base.

#ifndef RC_RC_MOBILE_MM_FRAMES_HPP
#define RC_RC_MOBILE_MM_FRAMES_HPP

#include <cmath>
#include <rc/arm/fk.hpp>
#include <rc/arm/frames.hpp>
#include <rc/arm/ik_closed.hpp>
#include <vector>

namespace rc {
namespace mobile {

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


}  // namespace mobile
}  // namespace rc

#endif
