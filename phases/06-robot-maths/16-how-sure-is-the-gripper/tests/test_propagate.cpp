#include <rc/test/rc_test.hpp>

#include <cmath>
#include <iomanip>
#include <iostream>
#include <vector>

#include "solution.hpp"

namespace {

const double kL1 = 0.5;
const double kL2 = 0.4;
const double kPi = 3.14159265358979323846;

double rel(double a, double b) { return std::fabs(a - b) / std::fmax(1e-12, std::fabs(b)); }

}  // namespace

RC_TEST("the tool is where the two links put it") {
  const Point2 straight = tool_position(kL1, kL2, 0.0, 0.0);
  RC_CHECK_NEAR(straight.x, 0.9, 1e-12);
  RC_CHECK_NEAR(straight.y, 0.0, 1e-12);
  const Point2 up = tool_position(kL1, kL2, kPi / 2.0, 0.0);
  RC_CHECK_NEAR(up.x, 0.0, 1e-12);
  RC_CHECK_NEAR(up.y, 0.9, 1e-12);
  const Point2 elbow = tool_position(kL1, kL2, 0.0, kPi / 2.0);
  RC_CHECK_NEAR(elbow.x, 0.5, 1e-12);
  RC_CHECK_NEAR(elbow.y, 0.4, 1e-12);
}

RC_TEST("the Jacobian is how the tool moves for a small move of each joint, and agrees with moving the joints") {
  const double h = 1e-6;
  const double poses[][2] = {{0.3, 1.1}, {-0.8, 0.5}, {1.2, -1.4}};
  double worst = 0.0;
  for (const auto& pose : poses) {
    const Jac j = jacobian(kL1, kL2, pose[0], pose[1]);
    const Point2 p1a = tool_position(kL1, kL2, pose[0] + h, pose[1]);
    const Point2 p1b = tool_position(kL1, kL2, pose[0] - h, pose[1]);
    const Point2 p2a = tool_position(kL1, kL2, pose[0], pose[1] + h);
    const Point2 p2b = tool_position(kL1, kL2, pose[0], pose[1] - h);
    worst = std::fmax(worst, std::fabs(j.a - (p1a.x - p1b.x) / (2 * h)));
    worst = std::fmax(worst, std::fabs(j.c - (p1a.y - p1b.y) / (2 * h)));
    worst = std::fmax(worst, std::fabs(j.b - (p2a.x - p2b.x) / (2 * h)));
    worst = std::fmax(worst, std::fabs(j.d - (p2a.y - p2b.y) / (2 * h)));
  }
  std::cout << "  the Jacobian against a central difference, three poses: worst difference " << std::scientific << std::setprecision(1) << worst << "\n";
  RC_CHECK(worst < 1e-8);
  // And the numbers are not both zero: at 0.3 and 1.1 the shoulder moves the tool by about half a metre per radian.
  const Jac known = jacobian(kL1, kL2, 0.3, 1.1);
  RC_CHECK(std::fabs(known.a) > 0.1);
  RC_CHECK(std::fabs(known.c) > 0.1);
}

RC_TEST("the doubt at the tool is J C J transposed, worked out for a pose") {
  // With the shoulder at 0 and the elbow at 90 degrees and links of 1: J = [[-1, -1], [1, 0]].
  const Jac j = jacobian(1.0, 1.0, 0.0, kPi / 2.0);
  RC_CHECK_NEAR(j.a, -1.0, 1e-12);
  RC_CHECK_NEAR(j.b, -1.0, 1e-12);
  RC_CHECK_NEAR(j.c, 1.0, 1e-12);
  RC_CHECK_NEAR(j.d, 0.0, 1e-12);

  Cov2 joint;
  joint.xx = 0.04;          // a spread of 0.2 rad in the shoulder
  joint.yy = 0.09;          // 0.3 rad in the elbow
  joint.xy = 0.0;
  const Cov2 tool = propagate(j, joint);
  // x = -q1 - q2: variance 0.04 + 0.09; y = q1: variance 0.04; covariance of (-q1 - q2) and q1 is -0.04.
  RC_CHECK_NEAR(tool.xx, 0.13, 1e-12);
  RC_CHECK_NEAR(tool.yy, 0.04, 1e-12);
  RC_CHECK_NEAR(tool.xy, -0.04, 1e-12);

  // Errors that move together in the joints: a correlation changes the answer.
  joint.xy = 0.02;
  const Cov2 together = propagate(j, joint);
  RC_CHECK_NEAR(together.xx, 0.04 + 0.09 + 2.0 * 0.02, 1e-12);
}

RC_TEST("for small joint errors the propagated doubt matches twenty thousand random trials, and for large ones it does not") {
  const double q1 = 0.4;
  const double q2 = 1.0;
  std::cout << "\n  a pose with the shoulder at 0.4 and the elbow at 1.0 rad; the covariance at the tool, linear propagation against 20000 random trials:\n"
            << "    joint spread   variance in x          variance in y          covariance\n"
            << "                   linear    sampled      linear    sampled      linear    sampled\n";
  double small_error = 0.0;
  double large_error = 0.0;
  const double spreads[] = {0.01, 0.05, 0.3};
  for (double s : spreads) {
    Cov2 joint;
    joint.xx = s * s;
    joint.yy = s * s;
    const Cov2 lin = propagate(jacobian(kL1, kL2, q1, q2), joint);
    const Cov2 mc = sampled_tool_covariance(kL1, kL2, q1, q2, s, s, 20000, 42);
    std::cout << "    " << std::fixed << std::setprecision(2) << std::setw(8) << s << std::scientific << std::setprecision(2) << "     "
              << lin.xx << "  " << mc.xx << "   " << lin.yy << "  " << mc.yy << "   " << lin.xy << "  " << mc.xy << "\n";
    const double e = std::fmax(rel(lin.xx, mc.xx), std::fmax(rel(lin.yy, mc.yy), std::fabs(lin.xy - mc.xy) / std::fabs(mc.xx)));
    if (s == 0.01) small_error = e;
    if (s == 0.3) large_error = e;
  }
  std::cout << "\n";
  RC_CHECK(small_error < 0.05);
  RC_CHECK(large_error > 0.1);
}

RC_TEST("the ellipse from the propagated doubt holds the share of random trials it should") {
  const double q1 = 0.4;
  const double q2 = 1.0;
  const double s = 0.02;
  Cov2 joint;
  joint.xx = s * s;
  joint.yy = s * s;
  const Jac j = jacobian(kL1, kL2, q1, q2);
  const Cov2 lin = propagate(j, joint);
  const Point2 centre = tool_position(kL1, kL2, q1, q2);

  rc::core::Rng rng(7);
  int inside = 0;
  const int n = 20000;
  for (int i = 0; i < n; ++i) {
    const Point2 p = tool_position(kL1, kL2, q1 + s * rng.gaussian(), q2 + s * rng.gaussian());
    if (rc::math::inside_ellipse(p, centre, lin, rc::math::kInside95)) ++inside;
  }
  const double share = static_cast<double>(inside) / n;
  std::cout << "  the 95 percent ellipse of the propagated doubt held " << std::fixed << std::setprecision(1) << 100.0 * share << " percent of " << n
            << " trials, from joint errors of " << std::setprecision(2) << s << " rad\n\n";
  RC_CHECK_NEAR(share, 0.95, 0.01);
}

RC_TEST("near a straight arm the doubt at the tool is nearly all along one direction, and with the elbow square it is not") {
  Cov2 joint;
  joint.xx = 0.02 * 0.02;
  joint.yy = 0.02 * 0.02;
  std::cout << "  joint errors of 0.02 rad each, the shoulder at 0.4: spread of the tool along and across its long axis\n"
            << "    elbow        along (mm)   across (mm)   ratio\n";
  const double elbows[] = {kPi / 2.0, 0.5, 0.05};
  double ratio_square = 0.0;
  double ratio_straight = 0.0;
  for (double elbow : elbows) {
    const Cov2 c = propagate(jacobian(kL1, kL2, 0.4, elbow), joint);
    const rc::math::Axes a = rc::math::principal_axes(c);
    const double along = std::sqrt(a.major) * 1000.0;
    const double across = std::sqrt(std::fmax(0.0, a.minor)) * 1000.0;
    const double ratio = across > 1e-9 ? along / across : 1e9;
    std::cout << "    " << std::fixed << std::setprecision(2) << std::setw(8) << elbow << std::setprecision(2) << std::setw(14) << along
              << std::setw(14) << across << std::setprecision(1) << std::setw(9) << ratio << "\n";
    if (elbow == kPi / 2.0) ratio_square = ratio;
    if (elbow == 0.05) ratio_straight = ratio;
  }
  std::cout << "\n";
  RC_CHECK(ratio_square < 3.0);
  RC_CHECK(ratio_straight > 8.0);
  RC_CHECK(ratio_straight > 4.0 * ratio_square);
}
