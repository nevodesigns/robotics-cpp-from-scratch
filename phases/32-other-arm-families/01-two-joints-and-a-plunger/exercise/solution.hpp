#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cmath>

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
  (void)s;  // TODO: cos q2 = (x2 + y2 - l1 squared - l2 squared) / (2 l1 l2), false if outside [-1, 1]; q2 = elbow times acos of it; q1 = atan2(y, x) - atan2(l2 sin q2, l1 + l2 cos q2); false if either is outside its limits
  (void)x;
  (void)y;
  (void)elbow;
  q1 = 0.0;
  q2 = 0.0;
  return false;
}

// The determinant of the planar Jacobian: zero when the arm is straight or folded.
inline double scara_det(const Scara& s, double q2) { return s.l1 * s.l2 * std::sin(q2); }

// The torque about each of the first two (vertical) axes from a payload of `mass` kg: gravity is along them.
inline double gravity_torque(double mass) {
  (void)mass;
  return 0.0;
}


#endif
