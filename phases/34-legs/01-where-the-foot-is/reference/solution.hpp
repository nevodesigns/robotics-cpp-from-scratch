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
  const double off = g.side * g.l0;
  const double r2 = y * y + z * z;
  if (r2 < off * off) return false;
  const double d = std::sqrt(r2 - off * off);  // how far the foot is below the hip, in the leg's own plane
  q[0] = std::atan2(z, y) - std::atan2(-d, off);
  const double dd = x * x + d * d;
  const double c2 = (dd - g.l1 * g.l1 - g.l2 * g.l2) / (2.0 * g.l1 * g.l2);
  if (c2 > 1.0 || c2 < -1.0) return false;
  q[2] = (knee >= 0 ? 1.0 : -1.0) * std::acos(c2);
  q[1] = std::atan2(x, d) - std::atan2(g.l2 * std::sin(q[2]), g.l1 + g.l2 * std::cos(q[2]));
  return true;
}

// How far the leg is from straight: the sine of the knee angle, zero when the leg is fully out or folded.
inline double straightness(double q2) { return std::fabs(std::sin(q2)); }


#endif
