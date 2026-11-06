#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cmath>

#include <rc/kin/gravity.hpp>

using rc::kin::ArmBody;
using rc::kin::holding_torques;
using rc::kin::JointTorques;

// How hard each link is to spin about its own centre of mass, in kilograms times
// metres squared. A uniform rod is its mass times its length squared over twelve.
struct LinkInertia {
  double i1 = 0.0417;
  double i2 = 0.0160;
};

// How hard the arm is to accelerate: torque = M * acceleration. It is symmetric, and
// it depends on the elbow angle and on nothing else.
struct MassMatrix {
  double m11 = 0.0;
  double m12 = 0.0;
  double m22 = 0.0;
};

inline MassMatrix mass_matrix(const ArmBody& a, const LinkInertia& inertia, double q2, double payload) {
  // TODO: with c = cos(q2), and p the payload:
  //
  //   m11 = m1 c1^2 + m2 (l1^2 + c2^2 + 2 l1 c2 c) + i1 + i2 + p (l1^2 + l2^2 + 2 l1 l2 c)
  //   m12 = m2 (c2^2 + l1 c2 c) + i2 + p (l2^2 + l1 l2 c)
  //   m22 = m2 c2^2 + i2 + p l2^2
  //
  // Each term is a mass times a distance squared: a mass far from the axis is hard
  // to swing, and the cosine is how folding the elbow pulls the second link in.
  (void)a;
  (void)inertia;
  (void)q2;
  (void)payload;
  return MassMatrix{};
}

// The torque each motor must supply to give the joints this acceleration at this
// pose and speed, with a payload.
inline JointTorques inverse_dynamics(const ArmBody& a, const LinkInertia& inertia, double q1, double q2,
                                     double v1, double v2, double a1, double a2, double payload) {
  // TODO: three parts.
  //
  //   inertia      M times the accelerations (a1, a2)
  //   velocity     with h = (m2 c2 + p l2) l1 sin(q2):
  //                  shoulder gets  -h (2 v1 v2 + v2^2)
  //                  elbow gets     +h v1^2
  //   weight       holding_torques(a, q1, q2, payload), which is what is left at rest
  //
  //   shoulder = m11 a1 + m12 a2 - h (2 v1 v2 + v2^2) + weight.shoulder
  //   elbow    = m12 a1 + m22 a2 + h v1^2             + weight.elbow
  (void)a;
  (void)inertia;
  (void)q1;
  (void)q2;
  (void)v1;
  (void)v2;
  (void)a1;
  (void)a2;
  (void)payload;
  (void)holding_torques;
  return JointTorques{};
}

#endif  // LESSON_SOLUTION_HPP
