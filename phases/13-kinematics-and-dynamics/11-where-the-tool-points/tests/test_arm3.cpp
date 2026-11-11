#include <rc/test/rc_test.hpp>

#include <cmath>
#include <iostream>

#include "solution.hpp"

namespace {
const double kPi = 3.14159265358979323846;
}

RC_TEST("the tool of a three link arm is where the links put it, and points along the sum of the angles") {
  const Pose2 straight = forward3(1.0, 1.0, 1.0, 0.0, 0.0, 0.0);
  RC_CHECK_NEAR(straight.x, 3.0, 1e-12);
  RC_CHECK_NEAR(straight.y, 0.0, 1e-12);
  RC_CHECK_NEAR(straight.phi, 0.0, 1e-12);

  const Pose2 bent = forward3(1.0, 1.0, 1.0, kPi / 2.0, -kPi / 2.0, 0.0);   // up, then along the x axis for the other two
  RC_CHECK_NEAR(bent.x, 2.0, 1e-12);
  RC_CHECK_NEAR(bent.y, 1.0, 1e-12);
  RC_CHECK_NEAR(bent.phi, 0.0, 1e-12);

  const Pose2 folded = forward3(1.0, 1.0, 1.0, 0.0, kPi, kPi);                  // out, back, out again
  RC_CHECK_NEAR(folded.x, 1.0, 1e-12);
  RC_CHECK_NEAR(folded.phi, 2.0 * kPi, 1e-12);                                   // the heading is the sum: not wrapped here
  const Pose2 lengths = forward3(2.0, 1.0, 0.5, 0.0, 0.0, 0.0);
  RC_CHECK_NEAR(lengths.x, 3.5, 1e-12);
}

RC_TEST("a heading error is the short way round, where the difference of the numbers is the long way") {
  const double raw = 3.14 - (-3.14);
  const double wrapped = heading_error(3.14, -3.14);
  std::cout << "  headings 3.14 and -3.14: the difference is " << raw << ", the short way round " << wrapped << "\n\n";
  RC_CHECK_NEAR(raw, 6.28, 1e-12);
  RC_CHECK_NEAR(wrapped, 6.28 - 2.0 * kPi, 1e-9);
  RC_CHECK(std::fabs(wrapped) < 0.01);
  RC_CHECK_NEAR(heading_error(0.1, 0.3), -0.2, 1e-12);
  RC_CHECK_NEAR(heading_error(0.3, 0.1), 0.2, 1e-12);
  RC_CHECK_NEAR(heading_error(0.0, kPi / 2.0), -kPi / 2.0, 1e-12);
  for (double t = -7.0; t <= 7.0; t += 0.37) {
    const double e = heading_error(t, 0.5);
    RC_CHECK(e > -kPi - 1e-12 && e <= kPi + 1e-12);                           // always inside one turn
  }
}

RC_TEST("both solutions of a pose put the tool on it, to the precision of the arithmetic") {
  int checked = 0;
  int off = 0;
  for (double x = -1.8; x <= 1.8; x += 0.3) {
    for (double y = -1.8; y <= 1.8; y += 0.3) {
      for (double phi = -3.0; phi <= 3.0; phi += 0.5) {
        const Pose2 target{x, y, phi};
        const auto solutions = solve3(target, 1.0, 1.0, 1.0);
        if (!solutions) continue;
        const Arm3Solution both[2] = {solutions.value().elbow_up, solutions.value().elbow_down};
        for (const Arm3Solution& s : both) {
          const Pose2 got = forward3(1.0, 1.0, 1.0, s.q1, s.q2, s.q3);
          ++checked;
          if (std::fabs(got.x - x) > 1e-9 || std::fabs(got.y - y) > 1e-9 || std::fabs(heading_error(phi, got.phi)) > 1e-9) ++off;
        }
      }
    }
  }
  std::cout << "  " << checked << " solutions of reachable poses: " << off << " off by more than 1e-9\n\n";
  RC_CHECK(checked > 500);
  RC_CHECK_EQ(off, 0);
}

RC_TEST("a point inside the reach of the whole arm can be reachable pointing one way and not pointing the other") {
  const auto along = solve3(Pose2{2.9, 0.0, 0.0}, 1.0, 1.0, 1.0);          // the last link points on, so the wrist is at 1.9
  const auto back = solve3(Pose2{2.9, 0.0, kPi}, 1.0, 1.0, 1.0);           // the last link points back, so the wrist would be at 3.9
  std::cout << "  the point (2.9, 0), links 1, 1, 1 (total reach 3): pointing along x it is " << (along ? "reachable" : "not reachable") << ", pointing back it is "
            << (back ? "reachable" : "not reachable") << "\n";
  RC_CHECK(static_cast<bool>(along));
  RC_CHECK(!back);
  if (!back) RC_CHECK(back.error() == rc::kin::ReachError::TooFar);
  RC_CHECK(static_cast<bool>(solve3(Pose2{0.1, 0.0, 0.0}, 1.0, 1.0, 1.0)));   // close to the base is fine: the wrist is in the middle of the two link disc
  RC_CHECK(!solve3(Pose2{3.2, 0.0, 0.0}, 1.0, 1.0, 1.0));                      // beyond the whole arm in any heading
  // The same arm with a long last link: the wrist is inside the hole that the first two cannot reach into, for some headings.
  const auto hole = solve3(Pose2{0.0, 0.0, 0.0}, 2.0, 1.0, 0.5);
  RC_CHECK(!hole);                                                              // wrist at (-0.5, 0): inside the radius 1 of the shorter link
}

RC_TEST("of the headings a tool can have at one point, only some can be reached, and how many is a sum of the two disc") {
  int reachable = 0;
  for (int degrees = 0; degrees < 360; ++degrees) {
    const double phi = degrees * kPi / 180.0;
    if (solve3(Pose2{1.5, 0.0, phi}, 1.0, 1.0, 1.0)) ++reachable;
  }
  std::cout << "  at (1.5, 0) with links 1, 1, 1: " << reachable << " of 360 headings can be reached\n\n";
  RC_CHECK_EQ(reachable, 209);                                                  // the wrist must be within 2 of the base: cos(phi) >= -0.25
  int at_base = 0;
  for (int degrees = 0; degrees < 360; ++degrees)
    if (solve3(Pose2{2.0, 0.0, degrees * kPi / 180.0}, 1.0, 1.0, 1.0)) ++at_base;
  RC_CHECK(at_base < 209);                                                      // further out, fewer headings
  RC_CHECK(at_base > 0);
}
