#include <rc/test/rc_test.hpp>

#include <cmath>
#include <iostream>

#include "solution.hpp"

namespace {
const double kPi = 3.14159265358979323846;
const double kL = 1.0;

double norm3(const double v[3]) { return std::sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]); }
}  // namespace

RC_TEST("the Jacobian is how the pose changes with each angle, as a difference of the forward kinematics shows") {
  const double q[3] = {0.4, -0.7, 0.9};
  const Jac3 j = jacobian3(kL, 0.8, 0.5, q[0], q[1], q[2]);
  double worst = 0.0;
  for (int c = 0; c < 3; ++c) {
    double hi[3] = {q[0], q[1], q[2]}, lo[3] = {q[0], q[1], q[2]};
    const double h = 1e-6;
    hi[c] += h;
    lo[c] -= h;
    const Pose2 a = forward3(kL, 0.8, 0.5, hi[0], hi[1], hi[2]);
    const Pose2 b = forward3(kL, 0.8, 0.5, lo[0], lo[1], lo[2]);
    worst = std::fmax(worst, std::fabs((a.x - b.x) / (2 * h) - j.m[0][c]));
    worst = std::fmax(worst, std::fabs((a.y - b.y) / (2 * h) - j.m[1][c]));
    worst = std::fmax(worst, std::fabs((a.phi - b.phi) / (2 * h) - j.m[2][c]));
  }
  std::cout << "  the largest difference between the Jacobian and central differences: " << worst << "\n\n";
  RC_CHECK(worst < 1e-6);
  RC_CHECK_NEAR(j.m[2][0], 1.0, 1e-15);                 // the heading changes one for one with every angle
  RC_CHECK_NEAR(j.m[2][1], 1.0, 1e-15);
  RC_CHECK_NEAR(j.m[2][2], 1.0, 1e-15);
}

RC_TEST("damped steps arrive at poses that can be reached, from starts all round, and the arm that gets there is on the pose") {
  int starts = 0, converged = 0, off_pose = 0, total_iterations = 0, worst_iterations = 0;
  const Pose2 targets[] = {{1.5, 0.5, 0.3}, {0.5, 1.5, 1.2}, {-1.0, 1.0, 2.5}, {1.0, -1.2, -0.8}};
  for (const Pose2& target : targets) {
    for (double a = -2.5; a <= 2.5; a += 1.0) {
      for (double b = -2.0; b <= 2.0; b += 1.0) {
        const double start[3] = {a, b, 0.3};
        const NumericResult r = solve_numeric(target, start, kL, kL, kL, NumericOptions());
        ++starts;
        if (r.converged) {
          ++converged;
          total_iterations += r.iterations;
          worst_iterations = std::max(worst_iterations, r.iterations);
          const Pose2 got = forward3(kL, kL, kL, r.q[0], r.q[1], r.q[2]);
          if (std::fabs(got.x - target.x) > 1e-5 || std::fabs(got.y - target.y) > 1e-5 || std::fabs(heading_error(target.phi, got.phi)) > 1e-5) ++off_pose;
        }
      }
    }
  }
  std::cout << "  " << starts << " starts: " << converged << " converged, " << off_pose << " of those not on the pose, " << (converged ? total_iterations / converged : 0) << " iterations on average, the worst " << worst_iterations << "\n\n";
  RC_CHECK_EQ(off_pose, 0);
  RC_CHECK(converged >= starts * 9 / 10);
  RC_CHECK(worst_iterations <= 200);
}

RC_TEST("near a singularity the plain step is enormous, and the damped step is small and limited") {
  const double q[3] = {0.3, 1e-5, 0.0};                 // the arm very nearly straight: the links nearly line up
  const Jac3 j = jacobian3(kL, kL, kL, q[0], q[1], q[2]);
  const double ex = 0.0, ey = 0.0, eh = 0.0;
  (void)ex;
  (void)ey;
  (void)eh;
  // A request to move the tool in towards the base, along the arm: the direction that a straight arm cannot move in.
  const double inwards_x = -std::cos(q[0]) * 0.1, inwards_y = -std::sin(q[0]) * 0.1;
  const JointStep plain = dls_step(j, inwards_x, inwards_y, 0.0, 0.0, 1e12);
  const JointStep damped = dls_step(j, inwards_x, inwards_y, 0.0, 0.05, 0.3);
  const double plain_length = plain.ok ? norm3(plain.dq) : -1.0;
  const double damped_length = damped.ok ? norm3(damped.dq) : -1.0;
  std::cout << "  an arm 1e-5 rad from straight, asked to move the tool 0.1 along itself: the plain step is " << plain_length << " rad, the damped step " << damped_length << " rad\n\n";
  RC_CHECK(plain.ok);
  RC_CHECK(plain_length > 10.0);                          // an enormous step for a tenth of a metre
  RC_CHECK(damped.ok);
  RC_CHECK(damped_length <= 0.3 + 1e-12);                 // limited
  RC_CHECK(damped_length > 0.0);
  // Away from the singularity the damped step is the plain step, nearly: a small amount of damping costs a little accuracy.
  const double bent[3] = {0.4, 1.2, -0.5};
  const Jac3 jb = jacobian3(kL, kL, kL, bent[0], bent[1], bent[2]);
  const JointStep p2 = dls_step(jb, 0.01, -0.01, 0.005, 0.0, 1e12);
  const JointStep d2 = dls_step(jb, 0.01, -0.01, 0.005, 0.01, 1e12);
  RC_CHECK(p2.ok && d2.ok);
  for (int c = 0; c < 3; ++c) RC_CHECK(std::fabs(p2.dq[c] - d2.dq[c]) < 0.05 * (std::fabs(p2.dq[c]) + 1e-3));
}

RC_TEST("a pose that cannot be reached stops at the iteration limit with the tool as near as it can get, and is not a crash or a number that is not a number") {
  // Links 1, 1, 1: the tool at (2.9, 0) pointing back along x needs the wrist at 3.9, which the first two links cannot reach.
  const Pose2 impossible{2.9, 0.0, kPi};
  const double start[3] = {0.3, 0.5, 0.2};
  NumericOptions options;
  options.max_iterations = 300;
  const NumericResult r = solve_numeric(impossible, start, kL, kL, kL, options);
  std::cout << "  an unreachable pose: converged " << r.converged << " after " << r.iterations << " iterations, position error " << r.position_error << " m, heading error " << r.heading_error << " rad\n";
  RC_CHECK(!r.converged);
  RC_CHECK_EQ(r.iterations, 300);
  RC_CHECK(std::isfinite(r.q[0]) && std::isfinite(r.q[1]) && std::isfinite(r.q[2]));
  RC_CHECK(r.position_error > 1e-3 || std::fabs(r.heading_error) > 1e-3);       // it did not arrive
  RC_CHECK(r.position_error < 2.0);                                              // it did not run away either
  // A pose beyond the whole arm: it stretches out towards it.
  const NumericResult far = solve_numeric(Pose2{5.0, 0.0, 0.0}, start, kL, kL, kL, options);
  RC_CHECK(!far.converged);
  RC_CHECK(far.position_error < 2.1);                                            // 5 - 3 = 2 at best
  RC_CHECK(far.position_error > 1.9);
  const Pose2 end = forward3(kL, kL, kL, far.q[0], far.q[1], far.q[2]);
  RC_CHECK(end.x > 2.5);                                                          // stretched out along x towards it
}

RC_TEST("with a limit on the elbow only one of the two solutions is found, and with limits that allow neither it stops where the limits stop it") {
  const Pose2 target{1.5, 0.5, 0.3};
  NumericOptions positive;
  positive.use_limits = true;
  positive.lo[1] = 0.1;
  positive.hi[1] = 3.0;                                    // the elbow bends one way only
  NumericOptions negative;
  negative.use_limits = true;
  negative.lo[1] = -3.0;
  negative.hi[1] = -0.1;                                   // and the other way only
  int both_ok = 0;
  double q2_positive = 0.0, q2_negative = 0.0;
  for (double q1 = -1.0; q1 <= 1.0; q1 += 0.5) {
    const double start_up[3] = {q1, 1.0, 0.0};
    const double start_down[3] = {q1, -1.0, 0.0};
    const NumericResult a = solve_numeric(target, start_up, kL, kL, kL, positive);
    const NumericResult b = solve_numeric(target, start_down, kL, kL, kL, negative);
    if (a.converged && b.converged) {
      ++both_ok;
      q2_positive = a.q[1];
      q2_negative = b.q[1];
    }
  }
  std::cout << "  the same pose with the elbow limited to bend one way and then the other: " << both_ok << " of 5 starts reached it both ways; the elbow angles were " << q2_positive << " and " << q2_negative << "\n";
  RC_CHECK(both_ok >= 3);
  RC_CHECK(q2_positive > 0.0);
  RC_CHECK(q2_negative < 0.0);
  RC_CHECK_NEAR(q2_positive, -q2_negative, 1e-3);          // the mirror solutions
  // Limits that leave no solution: the elbow cannot bend as far as the pose needs.
  NumericOptions stiff;
  stiff.use_limits = true;
  stiff.lo[1] = -0.05;
  stiff.hi[1] = 0.05;
  const double start[3] = {0.0, 0.0, 0.0};
  const NumericResult stuck = solve_numeric(Pose2{0.5, 0.5, 0.0}, start, kL, kL, kL, stiff);
  RC_CHECK(!stuck.converged);
  RC_CHECK(std::fabs(stuck.q[1]) <= 0.05 + 1e-12);         // the limit held at every step
}
