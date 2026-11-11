#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <algorithm>
#include <cmath>
#include <rc/kin/arm3.hpp>

// A formula for the angles of an arm exists for a few arms: the planar ones of the lessons before. Most arms have none that anyone can write down, and what a program does is take steps. It
// asks where the tool is for the angles it has, looks at how far that is from the pose it wants, asks how the tool moves when each angle moves (the Jacobian), and changes the angles by the
// amount that closes the gap. Repeated, it arrives, if there is anywhere to arrive.
//
// Three things decide whether that works. Near a singularity the Jacobian has no inverse, and the step the plain method asks for is enormous: damping keeps it small. A step is only a
// first order guess, so it has to be limited in size or it overshoots into a different part of the arm. And a pose that cannot be reached, or can only be reached outside the limits of the
// joints, must be told apart from one that has simply not been reached yet: the solver stops, and says which.

using rc::kin::forward3;
using rc::kin::heading_error;
using rc::kin::Pose2;

struct Jac3 {
  double m[3][3] = {{0, 0, 0}, {0, 0, 0}, {0, 0, 0}};   // rows: x, y, heading; columns: q1, q2, q3
};

// How the pose of the tool changes with each angle: the partial derivatives of x, y and the heading with respect to q1, q2 and q3, for the arm in this configuration.
inline Jac3 jacobian3(double l1, double l2, double l3, double q1, double q2, double q3) {
  // TODO: with s1 = sin(q1), s12 = sin(q1 + q2), s123 = sin(q1 + q2 + q3), and c1, c12, c123 for cos. Row x: -(l1 s1 + l2 s12 + l3 s123), -(l2 s12 + l3 s123), -(l3 s123).
  // Row y: l1 c1 + l2 c12 + l3 c123, l2 c12 + l3 c123, l3 c123. Row heading: 1, 1, 1.
  (void)l1;
  (void)l2;
  (void)l3;
  (void)q1;
  (void)q2;
  (void)q3;
  Jac3 j;
  j.m[0][0] = -99.0;
  return j;
}

struct JointStep {
  double dq[3] = {0.0, 0.0, 0.0};
  bool ok = false;     // false if the system could not be solved
};

// The change in the angles that closes the gap (ex, ey, eh) to first order, by damped least squares: dq = J^T (J J^T + lambda^2 I)^-1 e. With lambda 0 it is the plain inverse, which is
// huge near a singularity. A step whose length (as a vector of three angles) is more than `max_step` is scaled down to it. ok is false if the matrix has no inverse or the step is not a number.
inline JointStep dls_step(const Jac3& j, double ex, double ey, double eh, double lambda, double max_step) {
  // TODO: A = J J^T + lambda^2 I (3 by 3: A[r][c] is the sum over k of J[r][k] J[c][k], plus lambda^2 on the diagonal). Solve A y = e for e = (ex, ey, eh), by Cramer's rule or any way you like:
  // if the determinant is not above 1e-300 in size, return a step with ok false. dq = J^T y (dq[c] is the sum over rows r of J[r][c] y[r]). If its length is not finite, ok false. If its length is
  // above max_step, scale it down to max_step. Otherwise ok true.
  (void)j;
  (void)ex;
  (void)ey;
  (void)eh;
  (void)lambda;
  (void)max_step;
  return JointStep();
}

struct NumericOptions {
  double lambda = 0.05;          // the damping
  double max_step = 0.3;         // the longest step, in radians, as the length of the vector of the three
  int max_iterations = 200;
  double position_tolerance = 1e-6;
  double heading_tolerance = 1e-6;
  bool use_limits = false;       // keep each angle inside [lo, hi] after every step
  double lo[3] = {-3.14159265358979, -3.14159265358979, -3.14159265358979};
  double hi[3] = {3.14159265358979, 3.14159265358979, 3.14159265358979};
};

struct NumericResult {
  double q[3] = {0.0, 0.0, 0.0};
  int iterations = 0;
  bool converged = false;
  double position_error = 0.0;     // how far the tool is from the target position when it stopped
  double heading_error = 0.0;      // and from the target heading, the short way round
};

// Steps from `start` towards `target`, until the pose is within the tolerances (converged), or the iterations are used up, or a step cannot be taken. The result has the angles it
// stopped at and how far from the target the tool was there. With use_limits, every angle is put back inside its limits after each step.
inline NumericResult solve_numeric(const Pose2& target, const double (&start)[3], double l1, double l2, double l3, const NumericOptions& options) {
  // TODO: q starts as `start`. Repeat up to max_iterations times: now = forward3(...); ex = target.x - now.x, ey likewise, eh = heading_error(target.phi, now.phi). Record the position error (hypot of ex
  // and ey), the heading error and the iteration count in the result. If the position error is within position_tolerance and |eh| within heading_tolerance, converged = true: return. Otherwise
  // step = dls_step(jacobian3(...), ex, ey, eh, lambda, max_step); if it is not ok, stop. Add dq to each angle, and with use_limits put it back inside [lo, hi].
  (void)target;
  (void)start;
  (void)l1;
  (void)l2;
  (void)l3;
  (void)options;
  NumericResult r;
  r.position_error = -1.0;
  return r;
}

#endif  // LESSON_SOLUTION_HPP
