// rc/kin/singularity.hpp
//
// How far a pose is from the place where the arm cannot move slowly, and what
// to do about it, from lesson 13-04.
//
// For a two link planar arm the determinant of the Jacobian is l1 l2 sin(q2)
// exactly: it depends on the elbow and on nothing else, and it is zero with the
// arm straight. Measured on a 0.5 and 0.4 metre arm asked to move its tool
// outward at 0.1 m/s:
//
//   reach     elbow      det J     q1 rate    q2 rate
//   0.5000   1.9823   0.183303        0.09      -0.27
//   0.8500   0.6741   0.124844        0.30      -0.68
//   0.8990   0.0949   0.018947        2.11      -4.74
//   0.8999   0.0300   0.005999        6.67     -15.00
//
// A tenth of a millimetre from full extension, moving the tool a hundred
// millimetres a second needs the elbow at about a hundred and forty revolutions
// a minute, and a tenth of that distance again needs ten times as much.
//
// Damping bounds it, at a price. At that same pose, lambda of 0.01 gives a
// quarter of the joint rate and a quarter of the speed asked for. It does not
// get the arm through the singularity; it makes the arm refuse to go there
// quickly, which is why a real machine slows to a crawl near full extension.

#ifndef RC_KIN_SINGULARITY
#define RC_KIN_SINGULARITY

#include <cmath>

namespace rc {
namespace kin {

// The Jacobian of a two link planar arm: how the tool moves when the joints do.
//
// Four numbers, arranged as
//
//     [ a  b ]     dx = a * dq1 + b * dq2
//     [ c  d ]     dy = c * dq1 + d * dq2
//
// Lesson 13-03 built this by measuring, one joint at a time, because a general
// chain has no formula. A two link arm does, and having both is the point: the
// closed form here is what says exactly where the trouble is.
struct PlanarJacobian {
  double a = 0.0;
  double b = 0.0;
  double c = 0.0;
  double d = 0.0;

  double determinant() const { return a * d - b * c; }
};

inline PlanarJacobian planar_jacobian(double first_link, double second_link, double q1,
                                      double q2) {
  const double s1 = std::sin(q1);
  const double c1 = std::cos(q1);
  const double s12 = std::sin(q1 + q2);
  const double c12 = std::cos(q1 + q2);

  PlanarJacobian jacobian;
  jacobian.a = -first_link * s1 - second_link * s12;
  jacobian.b = -second_link * s12;
  jacobian.c = first_link * c1 + second_link * c12;
  jacobian.d = second_link * c12;
  return jacobian;
}

// How far this pose is from a place the arm cannot move slowly through.
//
// For this arm the determinant of the Jacobian works out to l1 * l2 * sin(q2)
// exactly, so it depends on the elbow and on nothing else. It is largest with
// the elbow square and zero with the arm straight, either fully extended or
// folded back on itself.
//
// Zero means the tool can no longer move in every direction at all: at full
// extension it cannot move outward, however fast the joints turn.
inline double manipulability(double first_link, double second_link, double q2) {
  return std::fabs(first_link * second_link * std::sin(q2));
}

// What the joints must do for the tool to move as asked.
struct JointRates {
  double first = 0.0;
  double second = 0.0;
  bool ok = false;
};

// Invert the Jacobian, or refuse.
//
// Refuse rather than divide, because the answer near a singularity is not
// merely large, it is unbounded. Measured on a 0.5 and 0.4 metre arm asked to
// move its tool outward at 0.1 m/s:
//
//   reach     elbow     det J      q1 rate    q2 rate
//   0.5000   1.9823   0.183303        0.09      -0.27
//   0.8500   0.6741   0.124844        0.30      -0.68
//   0.8990   0.0949   0.018947        2.11      -4.74
//   0.8999   0.0300   0.005999        6.67     -15.00
//
// A tenth of a millimetre from full extension, moving the tool a hundred
// millimetres a second needs the elbow at fifteen radians a second, which is
// about a hundred and forty revolutions a minute, and a tenth of that distance
// again needs ten times as much.
inline JointRates joint_rates(const PlanarJacobian& jacobian, double vx, double vy,
                              double least_determinant) {
  const double det = jacobian.determinant();
  if (std::fabs(det) <= least_determinant) return JointRates{};

  JointRates rates;
  rates.first = (jacobian.d * vx - jacobian.b * vy) / det;
  rates.second = (-jacobian.c * vx + jacobian.a * vy) / det;
  rates.ok = true;
  return rates;
}

// The same question asked in a way that always has an answer.
//
// Damped least squares: instead of solving J q = v exactly, find the q that
// minimises the tool error and the joint effort together, with lambda deciding
// the exchange rate. The matrix inverted is J J' + lambda^2 I, which cannot be
// singular for any lambda above zero.
//
// It does not get the arm through the singularity. It makes the arm refuse to
// go there quickly, which is the behaviour you want and is why a real machine
// slows to a crawl near full extension rather than throwing itself at the stop.
// Measured at a tenth of a millimetre from full extension, asked for 0.1 m/s:
//
//   lambda    q1 rate   q2 rate   tool speed given
//   0.000        6.67    -15.00            0.10000
//   0.010        1.80     -4.06            0.02707
//   0.050        0.10     -0.22            0.00149
inline JointRates damped_joint_rates(const PlanarJacobian& jacobian, double vx, double vy,
                                     double lambda) {
  // J J' + lambda^2 I, which is symmetric and two by two.
  const double squared = lambda * lambda;
  const double a = jacobian.a * jacobian.a + jacobian.b * jacobian.b + squared;
  const double b = jacobian.a * jacobian.c + jacobian.b * jacobian.d;
  const double d = jacobian.c * jacobian.c + jacobian.d * jacobian.d + squared;

  const double det = a * d - b * b;
  if (det == 0.0) return JointRates{};

  // Solve (J J' + lambda^2 I) w = v, then take q = J' w.
  const double w1 = (d * vx - b * vy) / det;
  const double w2 = (-b * vx + a * vy) / det;

  JointRates rates;
  rates.first = jacobian.a * w1 + jacobian.c * w2;
  rates.second = jacobian.b * w1 + jacobian.d * w2;
  rates.ok = true;
  return rates;
}

}  // namespace kin
}  // namespace rc

#endif  // RC_KIN_SINGULARITY
