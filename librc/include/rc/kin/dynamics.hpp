// rc/kin/dynamics.hpp
//
// What moving an arm costs, from lesson 13-07, graduated. It builds on
// rc/kin/gravity.hpp, which is what is left when the arm is not moving.
//
// Three things beyond weight: the inertia, which is a matrix because accelerating
// one joint takes torque at the other, and which changes with the elbow angle; the
// Coriolis and centripetal terms, which come from one joint's motion acting on the
// other; and the two together with the weight as the torque for a whole motion.
//
// Numbers from that lesson worth carrying. The inertia the shoulder sees was 4.0
// times smaller with the elbow folded than stretched, so one set of gains overshot
// by 1.9 percent at one pose and settled 0.22 s slower at the other. A shoulder swing
// of 1.0 rad in 0.4 s tracked to 281.9 mm worst with gravity fed forward and to 6.1
// mm with the inverse dynamics. And the model is checked by energy: a free swing at
// up to 38 rad/s drifted 1.5e-06 J when right, 26.6 J with a velocity term's sign
// wrong, and 151.8 J with one term left out.

#ifndef RC_KIN_DYNAMICS_HPP
#define RC_KIN_DYNAMICS_HPP

#include <cmath>

#include <rc/kin/gravity.hpp>

namespace rc {
namespace kin {

// How hard each link is to spin about its own centre of mass, in kilograms times
// metres squared. A uniform rod is its mass times its length squared over twelve,
// which is what these default to for the arm of lesson 13-06.
struct LinkInertia {
  double i1 = 0.0417;
  double i2 = 0.0160;
};

// How hard the arm is to accelerate: torque = M * acceleration.
//
// It is a matrix because the joints are not independent. Accelerating the shoulder
// takes torque at the elbow too (m12), and it is symmetric. It is not a constant: the
// inertia the shoulder sees depends on the elbow angle, because folding the elbow
// brings the second link's mass in towards the shoulder, and a mass close to the
// axis is easy to swing.
struct MassMatrix {
  double m11 = 0.0;
  double m12 = 0.0;
  double m22 = 0.0;
};

// Only the elbow angle appears. Rotating the whole arm about the shoulder does not
// change how its parts are arranged, so it cannot change how hard they are to move.
inline MassMatrix mass_matrix(const ArmBody& a, const LinkInertia& inertia, double q2, double payload) {
  const double c = std::cos(q2);

  MassMatrix m;
  m.m11 = a.m1 * a.c1 * a.c1 + a.m2 * (a.l1 * a.l1 + a.c2 * a.c2 + 2.0 * a.l1 * a.c2 * c) + inertia.i1 +
          inertia.i2 + payload * (a.l1 * a.l1 + a.l2 * a.l2 + 2.0 * a.l1 * a.l2 * c);
  m.m12 = a.m2 * (a.c2 * a.c2 + a.l1 * a.c2 * c) + inertia.i2 + payload * (a.l2 * a.l2 + a.l1 * a.l2 * c);
  m.m22 = a.m2 * a.c2 * a.c2 + inertia.i2 + payload * a.l2 * a.l2;
  return m;
}

// The torque each motor must supply to give the joints this acceleration at this
// pose and speed, with a payload.
//
// Three parts. The inertia times the acceleration. The Coriolis and centripetal
// terms, which exist only while the arm is moving and which come from one joint's
// motion twisting the other's frame: a swinging shoulder pulls the elbow outward,
// and an elbow turning while the shoulder turns adds a term of its own. And the
// weight, the holding torque of lesson 13-06, which is what is left at rest.
inline JointTorques inverse_dynamics(const ArmBody& a, const LinkInertia& inertia, double q1, double q2,
                                     double v1, double v2, double a1, double a2, double payload) {
  const MassMatrix m = mass_matrix(a, inertia, q2, payload);

  // One number, in both of the velocity terms: the mass beyond the elbow, times
  // the first link's length, times the sine of the elbow angle. It is zero when the
  // arm is straight or folded flat, which is why a stretched arm has no Coriolis
  // coupling and a bent one has a lot.
  const double h = (a.m2 * a.c2 + payload * a.l2) * a.l1 * std::sin(q2);

  const JointTorques weight = holding_torques(a, q1, q2, payload);

  JointTorques t;
  t.shoulder = m.m11 * a1 + m.m12 * a2 - h * (2.0 * v1 * v2 + v2 * v2) + weight.shoulder;
  t.elbow = m.m12 * a1 + m.m22 * a2 + h * v1 * v1 + weight.elbow;
  return t;
}

}  // namespace kin
}  // namespace rc

#endif  // RC_KIN_DYNAMICS_HPP
