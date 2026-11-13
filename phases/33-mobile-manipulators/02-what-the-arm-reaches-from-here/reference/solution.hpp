#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cmath>
#include <rc/arm/ik_closed.hpp>
#include <rc/arm/reach3d.hpp>
#include <rc/mobile/mm_frames.hpp>

using namespace rc::arm;
using namespace rc::mobile;

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


#endif
