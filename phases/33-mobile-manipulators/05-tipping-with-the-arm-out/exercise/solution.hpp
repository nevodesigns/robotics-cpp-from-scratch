#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cmath>
#include <rc/arm/fk.hpp>
#include <rc/mobile/mm_frames.hpp>
#include <rc/safety/tilt.hpp>
#include <vector>

using namespace rc::arm;
using namespace rc::mobile;

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
  (void)mm;  // TODO: the mean, weighted by mass, of the base's centre (in the world), the middle of each link (between the previous frame, the arm's base for the first, and its own) and the tool for the payload
  (void)b;
  (void)q;
  (void)mass;
  for (int k = 0; k < 3; ++k) out[k] = 0.0;
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


#endif
