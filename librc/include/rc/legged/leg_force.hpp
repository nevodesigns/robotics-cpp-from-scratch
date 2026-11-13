// rc/legged/leg_force.hpp
//
// What the foot can push with, from lesson 34-02.
//
// leg_jacobian gives the 3 by 3 matrix that turns the three joint rates into the foot's velocity in the hip's frame (the derivative of leg_fk). By the principle of virtual work, the generalised force on the joints from a force that the ground pushes the foot with is the Jacobian's transpose times it, so the torques the joints must give to balance it are minus that: stand_torques. foot_force_from_torques is the other way (minus the inverse of the transpose, found by Cramer's rule), and joint_rates is the inverse of the Jacobian applied to a foot velocity; both give false when the determinant is nearly zero, which is where the leg is straight or the lean is at its singular place.
//
// A robot of 12 kg standing on four legs puts 29.4 N on each foot. With the foot 0.20 m below the hip the knee had to give 5.43 N m and the hip 0.00 N m (the foot is directly under the hip, so the weight has no lever about it); at 0.30 m 4.33 N m at the knee; and at 0.40 m, with the leg nearly straight, 1.88 N m. The foot force found back from those torques was within 0.000000000 N. A foot speed of 0.5 m/s straight down needed a knee rate of 3.4 rad/s at 0.30 m and 15.5 rad/s at 0.40 m.

#ifndef RC_RC_LEGGED_LEG_FORCE_HPP
#define RC_RC_LEGGED_LEG_FORCE_HPP

#include <cmath>
#include <rc/legged/leg_kin.hpp>

namespace rc {
namespace legged {

// The matrix that gives the foot's velocity (hip's frame) for the three joint rates.
inline void leg_jacobian(const LegGeom& g, double q0, double q1, double q2, double j[3][3]) {
  const double a1 = g.l1 * std::cos(q1) + g.l2 * std::cos(q1 + q2);   // da/dq1
  const double a2 = g.l2 * std::cos(q1 + q2);                          // da/dq2
  const double b = -(g.l1 * std::cos(q1) + g.l2 * std::cos(q1 + q2));
  const double b1 = g.l1 * std::sin(q1) + g.l2 * std::sin(q1 + q2);   // db/dq1
  const double b2 = g.l2 * std::sin(q1 + q2);                          // db/dq2
  const double off = g.side * g.l0;
  const double c = std::cos(q0), s = std::sin(q0);
  j[0][0] = 0.0;
  j[0][1] = a1;
  j[0][2] = a2;
  j[1][0] = -off * s - b * c;
  j[1][1] = -s * b1;
  j[1][2] = -s * b2;
  j[2][0] = off * c - b * s;
  j[2][1] = c * b1;
  j[2][2] = c * b2;
}

// The torques at the three joints that hold a force `ground` (the force the ground pushes the foot with, in the hip's frame): minus the transpose of the Jacobian times it.
inline void stand_torques(const double j[3][3], const double ground[3], double tau[3]) {
  for (int c = 0; c < 3; ++c) tau[c] = -(j[0][c] * ground[0] + j[1][c] * ground[1] + j[2][c] * ground[2]);
}

inline double det3(const double m[3][3]) {
  return m[0][0] * (m[1][1] * m[2][2] - m[1][2] * m[2][1]) - m[0][1] * (m[1][0] * m[2][2] - m[1][2] * m[2][0]) + m[0][2] * (m[1][0] * m[2][1] - m[1][1] * m[2][0]);
}

// Solves m x = b for three unknowns by Cramer's rule; false if the determinant is nearly zero.
inline bool solve3(const double m[3][3], const double b[3], double x[3]) {
  const double d = det3(m);
  if (std::fabs(d) < 1e-5) return false;
  for (int k = 0; k < 3; ++k) {
    double t[3][3];
    for (int r = 0; r < 3; ++r)
      for (int c = 0; c < 3; ++c) t[r][c] = c == k ? b[r] : m[r][c];
    x[k] = det3(t) / d;
  }
  return true;
}

// The force the ground pushes the foot with, from the torques at the joints (the inverse of stand_torques); false near a singular leg.
inline bool foot_force_from_torques(const double j[3][3], const double tau[3], double ground[3]) {
  double jt[3][3], neg[3];
  for (int r = 0; r < 3; ++r) {
    neg[r] = -tau[r];
    for (int c = 0; c < 3; ++c) jt[r][c] = j[c][r];
  }
  return solve3(jt, neg, ground);
}

// The joint rates for a foot velocity; false near a singular leg.
inline bool joint_rates(const double j[3][3], const double v[3], double rates[3]) { return solve3(j, v, rates); }


}  // namespace legged
}  // namespace rc

#endif
