// rc/mobile/base_reach.hpp
//
// What the arm reaches from here, from lesson 33-02.
//
// arm_can_reach puts a target given in the world into the arm's frame (33-01) and asks the arm's inverse kinematics whether the tool can be put there pointing down, within the joint limits (28-08). reach_annulus finds, for a height in the arm's frame, the inner and outer radius of the ring of points the arm reaches: it steps outward along a line from the arm's axis and notes the first and the last radius reached. annulus_can_reach is the quick test: the arm's axis is the base's place moved by the mount, and the target is reachable if its ground distance from that axis lies between the two radii. park_tolerance is half the width of the ring: how far from the middle of the ring the base may stop and still reach.
//
// At a target height of -0.10 m in the arm's frame the arm reached a ring from 0.00 m to 0.72 m; the quick test agreed with the full inverse kinematics on 100 percent of 576 base places. The ring's width left a tolerance of 36.0 cm for the base's stopping place, against 31.5 cm at a height 0.20 m lower.

#ifndef RC_RC_MOBILE_BASE_REACH_HPP
#define RC_RC_MOBILE_BASE_REACH_HPP

#include <cmath>
#include <rc/arm/ik_closed.hpp>
#include <rc/arm/reach3d.hpp>
#include <rc/mobile/mm_frames.hpp>

namespace rc {
namespace mobile {

using rc::arm::ArmTable;
using rc::arm::Pose;
using rc::arm::tool_down_reachable;

struct Annulus {
  double r_in = 0.0;
  double r_out = 0.0;
};

// Can the arm put its tool at a point given in the world, pointing down, from this base pose?
inline bool arm_can_reach(const MobileManipulator& m, const BasePose& b, const double world[3]) {
  double p[3];
  world_to_arm(m, b, world, p);
  return tool_down_reachable(m.arm, p[0], p[1], p[2], m.tool);
}

// The ring the arm reaches at a height z in its own frame (tool down): the first and the last radius reached along a line out from its axis.
inline Annulus reach_annulus(const MobileManipulator& m, double z, double step) {
  Annulus a;
  bool any = false;
  for (double r = 0.0; r < 3.0; r += step) {
    if (tool_down_reachable(m.arm, r, 0.0, z, m.tool)) {
      if (!any) a.r_in = r;
      a.r_out = r;
      any = true;
    }
  }
  return a;
}

// The quick test: is the target's ground distance from the arm's axis inside the ring?
inline bool annulus_can_reach(const MobileManipulator& m, const Annulus& a, const BasePose& b, const double world[3]) {
  const Pose axis = arm_base_in_world(m, b);
  const double d = std::hypot(world[0] - axis.p[0], world[1] - axis.p[1]);
  return d >= a.r_in && d <= a.r_out;
}

// How far from the middle of the ring the base may stop.
inline double park_tolerance(const Annulus& a) { return 0.5 * (a.r_out - a.r_in); }


}  // namespace mobile
}  // namespace rc

#endif
