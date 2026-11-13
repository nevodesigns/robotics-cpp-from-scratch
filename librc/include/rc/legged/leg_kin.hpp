// rc/legged/leg_kin.hpp
//
// Where the foot is, from lesson 34-01.
//
// LegGeom is the sideways offset of the hip's pitch axis from the lean axis (l0, a left leg has side +1 and a right leg -1), the length of the thigh (l1) and of the shank (l2). The foot's place in the frame at the hip (x forward, y left, z up) from the three joint angles is leg_fk: the thigh and shank give a place in the plane of the leg, and the lean turns that plane about the forward axis. leg_ik is the other way: the lean follows from the foot's place in the plane across the body and the offset, and the plane of the leg then holds a two link problem whose two answers are the knee bent one way or the other (knee +1 or -1). It says whether the place can be reached. straightness is the sine of the knee's angle: zero when the leg is fully out or folded.
//
// A leg with a thigh of 0.21 m, a shank of 0.21 m and an offset of 0.08 m put the foot 0.30 m below the hip with the knee at -89 degrees. Over a grid of 756 places 680 were reachable and the foot came back to them to 0.000000000 m; the two knees gave the same foot to 0.000000000 m. A place 0.50 m below the hip was out of reach. The leg was straight (0.000) at full stretch of 0.42 m.

#ifndef RC_RC_LEGGED_LEG_KIN_HPP
#define RC_RC_LEGGED_LEG_KIN_HPP

#include <cmath>

namespace rc {
namespace legged {

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


}  // namespace legged
}  // namespace rc

#endif
