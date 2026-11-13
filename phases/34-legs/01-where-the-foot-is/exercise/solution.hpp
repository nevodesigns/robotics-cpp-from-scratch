#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cmath>

struct LegGeom {
  double l0 = 0.08;    // m, the sideways offset of the hip's pitch axis (typical of a 12 kg quadruped)
  double l1 = 0.21;    // m, the thigh
  double l2 = 0.21;    // m, the shank
  int side = 1;        // +1 for a left leg, -1 for a right one
};

struct FootPos {
  double x = 0.0, y = 0.0, z = 0.0;
};

// The foot in the frame at the hip (x forward, y left, z up) for a lean q0, a hip pitch q1 and a knee q2.
inline FootPos leg_fk(const LegGeom& g, double q0, double q1, double q2) {
  const double a = g.l1 * std::sin(q1) + g.l2 * std::sin(q1 + q2);
  const double b = -(g.l1 * std::cos(q1) + g.l2 * std::cos(q1 + q2));
  const double off = g.side * g.l0;
  FootPos p;
  p.x = a;
  p.y = off * std::cos(q0) - b * std::sin(q0);
  p.z = off * std::sin(q0) + b * std::cos(q0);
  return p;
}

// The joint angles that put the foot at (x, y, z), with the knee bent one way (+1) or the other (-1); false if it cannot be reached.
inline bool leg_ik(const LegGeom& g, double x, double y, double z, int knee, double q[3]) {
  (void)g;  // TODO: d = sqrt(y2 + z2 - offset squared) is the foot's depth in the leg's plane; q0 = atan2(z, y) - atan2(-d, offset); then the two link problem for (x, d): cos q2 = (x2 + d2 - l1 squared - l2 squared) / (2 l1 l2), q2 = knee times acos of it, q1 = atan2(x, d) - atan2(l2 sin q2, l1 + l2 cos q2); false if the place is out of reach
  (void)x;
  (void)y;
  (void)z;
  (void)knee;
  q[0] = 0.0;
  q[1] = 0.0;
  q[2] = 0.0;
  return false;
}

// How far the leg is from straight: the sine of the knee angle, zero when the leg is fully out or folded.
inline double straightness(double q2) { return std::fabs(std::sin(q2)); }


#endif
