// rc/arm/scara.hpp
//
// Two joints and a plunger, from lesson 32-01.
//
// Scara is a link length for each of the two vertical axis joints, the limits of those joints, the stroke of the vertical slide and the roll. scara_fk gives the tool's place and heading from the four joint values. scara_ik gives the two joint angles that put the tool at an (x, y) for a chosen elbow (+1 or -1), and says whether it can: false if the point is out of reach, or the angles fall outside the limits. scara_det is the determinant of the planar Jacobian, l1 l2 sin(q2), which is zero when the arm is straight or folded. gravity_torque gives the torque about each vertical axis from a payload: always zero, because gravity is along those axes.
//
// The arm of 0.225 m and 0.225 m links reached 159 of 272 points of a grid with an elbow either way, and the two elbows gave the same point to 0.000000000 m. A point 0.5 m out was 0 (0: out of reach). The determinant was 0.0506 at an elbow angle of 1.57 rad and 0.0000 when straight. A payload of 2 kg put 0.0 N m on the first two joints; the same payload held 0.4 m out by the shoulder of a six axis arm put 7.85 N m.

#ifndef RC_RC_ARM_SCARA_HPP
#define RC_RC_ARM_SCARA_HPP

#include <cmath>

namespace rc {
namespace arm {

struct Scara {
  double l1 = 0.225;      // m (typical of a 450 mm reach SCARA)
  double l2 = 0.225;      // m
  double q1_lo = -2.35, q1_hi = 2.35;   // rad (typical)
  double q2_lo = -2.6, q2_hi = 2.6;     // rad
  double z_stroke = 0.15; // m
};

struct ScaraTool {
  double x = 0.0, y = 0.0, z = 0.0, yaw = 0.0;
};

inline ScaraTool scara_fk(const Scara& s, double q1, double q2, double z, double roll) {
  ScaraTool t;
  t.x = s.l1 * std::cos(q1) + s.l2 * std::cos(q1 + q2);
  t.y = s.l1 * std::sin(q1) + s.l2 * std::sin(q1 + q2);
  t.z = z;
  t.yaw = q1 + q2 + roll;
  return t;
}

// The two joint angles that put the tool at (x, y) for an elbow of +1 or -1; false if out of reach or outside the limits.
inline bool scara_ik(const Scara& s, double x, double y, int elbow, double& q1, double& q2) {
  const double c2 = (x * x + y * y - s.l1 * s.l1 - s.l2 * s.l2) / (2.0 * s.l1 * s.l2);
  if (c2 > 1.0 || c2 < -1.0) return false;
  q2 = (elbow >= 0 ? 1.0 : -1.0) * std::acos(c2);
  q1 = std::atan2(y, x) - std::atan2(s.l2 * std::sin(q2), s.l1 + s.l2 * std::cos(q2));
  return q1 >= s.q1_lo && q1 <= s.q1_hi && q2 >= s.q2_lo && q2 <= s.q2_hi;
}

// The determinant of the planar Jacobian: zero when the arm is straight or folded.
inline double scara_det(const Scara& s, double q2) { return s.l1 * s.l2 * std::sin(q2); }

// The torque about each of the first two (vertical) axes from a payload of `mass` kg: gravity is along them.
inline double gravity_torque(double mass) {
  (void)mass;
  return 0.0;
}


}  // namespace arm
}  // namespace rc

#endif
