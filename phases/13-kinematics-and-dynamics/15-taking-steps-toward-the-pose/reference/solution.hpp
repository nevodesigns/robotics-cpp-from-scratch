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
  const double s1 = std::sin(q1), s12 = std::sin(q1 + q2), s123 = std::sin(q1 + q2 + q3);
  const double c1 = std::cos(q1), c12 = std::cos(q1 + q2), c123 = std::cos(q1 + q2 + q3);
  Jac3 j;
  j.m[0][0] = -(l1 * s1 + l2 * s12 + l3 * s123);
  j.m[0][1] = -(l2 * s12 + l3 * s123);
  j.m[0][2] = -(l3 * s123);
  j.m[1][0] = l1 * c1 + l2 * c12 + l3 * c123;
  j.m[1][1] = l2 * c12 + l3 * c123;
  j.m[1][2] = l3 * c123;
  j.m[2][0] = 1.0;
  j.m[2][1] = 1.0;
  j.m[2][2] = 1.0;
  return j;
}

struct JointStep {
  double dq[3] = {0.0, 0.0, 0.0};
  bool ok = false;     // false if the system could not be solved
};

// The change in the angles that closes the gap (ex, ey, eh) to first order, by damped least squares: dq = J^T (J J^T + lambda^2 I)^-1 e. With lambda 0 it is the plain inverse, which is
// huge near a singularity. A step whose length (as a vector of three angles) is more than `max_step` is scaled down to it. ok is false if the matrix has no inverse or the step is not a number.
inline JointStep dls_step(const Jac3& j, double ex, double ey, double eh, double lambda, double max_step) {
  double a[3][3];
  for (int r = 0; r < 3; ++r)
    for (int c = 0; c < 3; ++c) {
      double sum = 0.0;
      for (int k = 0; k < 3; ++k) sum += j.m[r][k] * j.m[c][k];
      a[r][c] = sum + (r == c ? lambda * lambda : 0.0);
    }
  const double det = a[0][0] * (a[1][1] * a[2][2] - a[1][2] * a[2][1]) - a[0][1] * (a[1][0] * a[2][2] - a[1][2] * a[2][0]) + a[0][2] * (a[1][0] * a[2][1] - a[1][1] * a[2][0]);
  JointStep step;
  if (!(std::fabs(det) > 1e-300)) return step;
  const double e[3] = {ex, ey, eh};
  double y[3];
  for (int col = 0; col < 3; ++col) {              // Cramer's rule
    double b[3][3];
    for (int r = 0; r < 3; ++r)
      for (int c = 0; c < 3; ++c) b[r][c] = (c == col) ? e[r] : a[r][c];
    y[col] = (b[0][0] * (b[1][1] * b[2][2] - b[1][2] * b[2][1]) - b[0][1] * (b[1][0] * b[2][2] - b[1][2] * b[2][0]) + b[0][2] * (b[1][0] * b[2][1] - b[1][1] * b[2][0])) / det;
  }
  for (int c = 0; c < 3; ++c) step.dq[c] = j.m[0][c] * y[0] + j.m[1][c] * y[1] + j.m[2][c] * y[2];
  const double length = std::sqrt(step.dq[0] * step.dq[0] + step.dq[1] * step.dq[1] + step.dq[2] * step.dq[2]);
  if (!std::isfinite(length)) return step;
  if (length > max_step) {
    const double scale = max_step / length;
    for (int c = 0; c < 3; ++c) step.dq[c] *= scale;
  }
  step.ok = true;
  return step;
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
  NumericResult r;
  for (int i = 0; i < 3; ++i) r.q[i] = start[i];
  for (int it = 0; it <= options.max_iterations; ++it) {
    const Pose2 now = forward3(l1, l2, l3, r.q[0], r.q[1], r.q[2]);
    const double ex = target.x - now.x, ey = target.y - now.y, eh = heading_error(target.phi, now.phi);
    r.position_error = std::hypot(ex, ey);
    r.heading_error = eh;
    r.iterations = it;
    if (r.position_error <= options.position_tolerance && std::fabs(eh) <= options.heading_tolerance) {
      r.converged = true;
      return r;
    }
    if (it == options.max_iterations) break;
    const JointStep step = dls_step(jacobian3(l1, l2, l3, r.q[0], r.q[1], r.q[2]), ex, ey, eh, options.lambda, options.max_step);
    if (!step.ok) break;
    for (int c = 0; c < 3; ++c) {
      r.q[c] += step.dq[c];
      if (options.use_limits) r.q[c] = std::max(options.lo[c], std::min(options.hi[c], r.q[c]));
    }
  }
  return r;
}

#endif  // LESSON_SOLUTION_HPP
