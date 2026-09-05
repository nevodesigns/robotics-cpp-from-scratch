#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cmath>

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

// TODO 1: the Jacobian of a two link planar arm.
//
// The tool is at
//
//     x = l1 cos(q1) + l2 cos(q1 + q2)
//     y = l1 sin(q1) + l2 sin(q1 + q2)
//
// so differentiating each with respect to each joint gives the four entries.
// Remember the chain rule on the second link: q1 + q2 depends on both.
//
// The test checks these against a central difference of the tool position, the
// way lesson 13-03 had to measure them, so a sign error will show immediately.
inline PlanarJacobian planar_jacobian(double first_link, double second_link, double q1,
                                      double q2) {
  (void)first_link;
  (void)second_link;
  (void)q1;
  (void)q2;
  return PlanarJacobian{};
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

// TODO 2: invert the Jacobian, or refuse.
//
// For a two by two matrix the inverse is the adjugate over the determinant:
//
//     q1 = ( d * vx - b * vy) / det
//     q2 = (-c * vx + a * vy) / det
//
// Refuse, returning an unset JointRates, when the determinant is no larger than
// least_determinant. Refuse rather than divide, because the answer near a
// singularity is not merely large, it is unbounded. Measured on a 0.5 and 0.4
// metre arm asked to move its tool outward at 0.1 m/s: 0.27 rad/s at half
// reach, 4.74 at a millimetre from full extension, 15.00 at a tenth of a
// millimetre, and ten times that again at a hundredth.
inline JointRates joint_rates(const PlanarJacobian& jacobian, double vx, double vy,
                              double least_determinant) {
  (void)jacobian;
  (void)vx;
  (void)vy;
  (void)least_determinant;
  return JointRates{};
}

// TODO 3: the same question asked in a way that always has an answer.
//
// Damped least squares. Instead of solving J q = v exactly, minimise the tool
// error and the joint effort together, which comes out as
//
//     q = J' (J J' + lambda^2 I)^-1 v
//
// J J' for this arm is the symmetric two by two
//
//     [ a*a + b*b     a*c + b*d ]
//     [ a*c + b*d     c*c + d*d ]
//
// so add lambda squared to the diagonal, invert that two by two, apply it to
// the velocity, and multiply the result by J transposed. It cannot be singular
// for any lambda above zero.
//
// This does not get the arm through the singularity. It makes the arm refuse to
// go there quickly, which is the behaviour you want. Measured a tenth of a
// millimetre from full extension, asked for 0.1 m/s: lambda 0.01 gives a
// quarter of the joint rate and a quarter of the speed.
inline JointRates damped_joint_rates(const PlanarJacobian& jacobian, double vx, double vy,
                                     double lambda) {
  (void)jacobian;
  (void)vx;
  (void)vy;
  (void)lambda;
  return JointRates{};
}

#endif  // LESSON_SOLUTION_HPP
