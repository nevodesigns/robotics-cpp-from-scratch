// rc/mobile/tip_margin.hpp
//
// Tipping with the arm out, from lesson 33-05 (it extends 19-09).
//
// MassModel holds the base's mass and centre of mass, the mass of each of the arm's links and a payload at the tool. com_world gives the combined centre of mass in the world for a base pose and joint angles: each link's mass is taken at the middle of the link (between its frame and the one before), the payload at the tool, and the centre is the mean of the places weighted by the masses. tip_limits puts that centre in the base's frame and gives the accelerations beyond which the base would tip: g times the distance of the centre from the edge of the rectangle of wheels (half a wheelbase fore and aft, half a track to each side) over the height of the centre. brake is the deceleration that tips it forward, accel the acceleration that tips it backward, and left and right the sideways accelerations (a turn to the left pushes the centre to the right).
//
// With the arm tucked in the centre of mass was -0.006 m forward of the base's centre at a height of 0.273 m and the base could brake at 11.01 m/s2; with the arm reaching 0.65 m forward it was 0.069 m forward at 0.255 m and the braking limit fell to 8.88 m/s2, and to 5.19 m/s2 with 5 kg in the hand. Reaching sideways the limit on turning towards the arm was 4.22 m/s2 against 7.92 tucked. From 1.0 m/s the loaded base needed at least 0.096 m to stop. The wheels on a dry floor (a friction of 0.6, typical) would let it brake at 5.89 m/s2, so with the load it tipped before it skidded.

#ifndef RC_RC_MOBILE_TIP_MARGIN_HPP
#define RC_RC_MOBILE_TIP_MARGIN_HPP

#include <cmath>
#include <rc/arm/fk.hpp>
#include <rc/mobile/mm_frames.hpp>
#include <rc/safety/tilt.hpp>
#include <vector>

namespace rc {
namespace mobile {

struct MassModel {
  double base_mass = 20.0;                 // kg (typical)
  double base_com[3] = {0.0, 0.0, 0.15};   // m, in the base frame
  std::vector<double> link_mass;           // kg, one for each of the arm's links
  double payload = 0.0;                    // kg, at the tool
};

struct TipLimits {
  double brake = 0.0;       // m/s2 of deceleration that tips the base forward
  double accel = 0.0;       // m/s2 of acceleration that tips it backward
  double left_turn = 0.0;   // m/s2 sideways (towards the left) that tips it to the right
  double right_turn = 0.0;  // m/s2 sideways (towards the right) that tips it to the left
  double com_x = 0.0;       // m, the centre of mass in the base frame
  double com_y = 0.0;
  double com_z = 0.0;
};

// The centre of mass of base, arm and payload in the world.
inline void com_world(const MobileManipulator& mm, const BasePose& b, const std::vector<double>& q, const MassModel& mass, double out[3]) {
  const rc::arm::Pose bp = base_pose(b);
  double sum[3];
  double bc[3];
  rc::arm::apply(bp, mass.base_com, bc);
  double total = mass.base_mass;
  for (int k = 0; k < 3; ++k) sum[k] = mass.base_mass * bc[k];
  const rc::arm::Pose axis = arm_base_in_world(mm, b);
  const std::vector<rc::arm::Pose> frames = rc::arm::link_frames(mm.arm, q, axis);
  double prev[3] = {axis.p[0], axis.p[1], axis.p[2]};
  for (std::size_t i = 0; i < frames.size() && i < mass.link_mass.size(); ++i) {
    for (int k = 0; k < 3; ++k) sum[k] += mass.link_mass[i] * 0.5 * (prev[k] + frames[i].p[k]);
    total += mass.link_mass[i];
    for (int k = 0; k < 3; ++k) prev[k] = frames[i].p[k];
  }
  const rc::arm::Pose tool = rc::arm::forward(mm.arm, q, axis, mm.tool);
  for (int k = 0; k < 3; ++k) sum[k] += mass.payload * tool.p[k];
  total += mass.payload;
  for (int k = 0; k < 3; ++k) out[k] = sum[k] / total;
}

// The accelerations beyond which the base tips, for a centre of mass in the world and the rectangle of the wheels (half a wheelbase and half a track).
inline TipLimits tip_limits(const BasePose& b, const double com[3], double half_wheelbase, double half_track) {
  TipLimits t;
  const double c = std::cos(b.heading), s = std::sin(b.heading);
  const double dx = com[0] - b.x, dy = com[1] - b.y;
  t.com_x = c * dx + s * dy;
  t.com_y = -s * dx + c * dy;
  t.com_z = com[2];
  const double g = rc::safety::kGravity;
  t.brake = g * (half_wheelbase - t.com_x) / t.com_z;
  t.accel = g * (half_wheelbase + t.com_x) / t.com_z;
  t.left_turn = g * (half_track + t.com_y) / t.com_z;
  t.right_turn = g * (half_track - t.com_y) / t.com_z;
  return t;
}

// The distance to stop from speed v at deceleration a.
inline double stopping_distance(double v, double a) { return a > 0.0 ? v * v / (2.0 * a) : 1e18; }


}  // namespace mobile
}  // namespace rc

#endif
