#include <rc/test/rc_test.hpp>

#include <rc/kin/chain.hpp>
#include <rc/math/transform.hpp>

#include <cmath>
#include <iomanip>
#include <iostream>
#include <vector>

#include "solution.hpp"

namespace {

constexpr double kL1 = 0.5;
constexpr double kL2 = 0.3;

// The arm from lesson 13-01, used here to check the answers rather than to
// produce them.
rc::kin::Chain planar_arm() {
  rc::kin::Chain arm;
  const rc::math::Vec3 z{0.0, 0.0, 1.0};
  arm.add(rc::kin::revolute_along_x(0.0, z));
  arm.add(rc::kin::revolute_along_x(kL1, z));
  arm.add(rc::kin::revolute_along_x(kL2, z));
  return arm;
}

// Where the arm actually ends up when told to hold this solution.
rc::math::Vec3 reached(const rc::kin::Chain& arm, const ArmSolution& s) {
  const std::vector<double> q{s.q1, s.q2, 0.0};
  return arm.tool(rc::span<const double>(q.data(), q.size())).translation;
}

}  // namespace

RC_TEST("a target straight ahead has an answer") {
  const auto solutions = solve(0.6, 0.0, kL1, kL2);
  RC_REQUIRE(solutions.has_value());
  RC_CHECK(!solutions.value().coincide);
}

RC_TEST("a target beyond the arm's reach is refused, not approximated") {
  const auto solutions = solve(1.5, 0.0, kL1, kL2);
  RC_REQUIRE(!solutions.has_value());
  RC_CHECK(solutions.error() == ReachError::TooFar);
}

RC_TEST("a target inside the hole the arm cannot fold into is refused") {
  // The links are 0.5 and 0.3, so nothing closer than 0.2 from the shoulder can
  // be reached however the elbow is bent.
  const auto solutions = solve(0.05, 0.0, kL1, kL2);
  RC_REQUIRE(!solutions.has_value());
  RC_CHECK(solutions.error() == ReachError::TooClose);
}

RC_TEST("both answers put the tool on the target") {
  // The round trip, and it is a fair one here for a specific reason. Lesson
  // 13-01 checked the forward kinematics against a closed form derived without
  // it, so the arm can now be trusted to say where a solution actually lands.
  // Round tripping through two halves that were never independently checked
  // proves only that they agree, which lesson 05-03 is about.
  const rc::kin::Chain arm = planar_arm();

  double worst = 0.0;
  int checked = 0;
  for (int i = -12; i <= 12; ++i) {
    for (int j = -12; j <= 12; ++j) {
      const double x = static_cast<double>(i) * 0.06;
      const double y = static_cast<double>(j) * 0.06;

      const auto solutions = solve(x, y, kL1, kL2);
      if (!solutions.has_value()) continue;

      for (const ArmSolution& s : {solutions.value().elbow_up, solutions.value().elbow_down}) {
        const rc::math::Vec3 tool = reached(arm, s);
        const double error = std::hypot(tool.x - x, tool.y - y);
        if (error > worst) worst = error;
        ++checked;
      }
    }
  }

  std::cout << "\n  " << checked << " solutions driven through the arm, worst miss "
            << std::scientific << std::setprecision(2) << worst << " metres\n";
  RC_CHECK(worst < 1e-12);
  RC_CHECK(checked > 200);
}

RC_TEST("the two answers are genuinely different, and are not the same arm twice") {
  // The check that catches a solver returning one answer under two names. An
  // arm that only ever bends one way will hit whatever the other way was
  // avoiding.
  const auto solutions = solve(0.5, 0.2, kL1, kL2);
  RC_REQUIRE(solutions.has_value());
  const Solutions& both = solutions.value();

  RC_CHECK(!both.coincide);
  RC_CHECK(std::fabs(both.elbow_up.q2 - both.elbow_down.q2) > 0.1);
  RC_CHECK(both.elbow_up.q2 * both.elbow_down.q2 < 0.0);   // they bend opposite ways
}

RC_TEST("at full extension the two answers become one") {
  const auto solutions = solve(kL1 + kL2, 0.0, kL1, kL2);
  RC_REQUIRE(solutions.has_value());

  const Solutions& both = solutions.value();
  RC_CHECK(both.coincide);
  RC_CHECK_NEAR(both.elbow_up.q2, 0.0, 1e-7);
  RC_CHECK_NEAR(both.elbow_down.q2, 0.0, 1e-7);
}

RC_TEST("a target computed from the arm itself at full extension still solves") {
  // The check that catches a missing clamp, and it is written this way on
  // purpose. Typing 0.8 gives an argument of exactly one. Asking the arm where
  // it is when fully extended gives a target whose argument is 1 + 7e-16, and
  // acos of that is not a number.
  //
  // Measured across a hundred thousand fully extended configurations, more than
  // half produced an argument above one, so this is the common case at the
  // workspace boundary rather than an unlucky one.
  const rc::kin::Chain arm = planar_arm();

  int solved = 0;
  int attempted = 0;
  for (int i = 0; i < 400; ++i) {
    const double q1 = static_cast<double>(i) * 0.0157;
    const std::vector<double> straight{q1, 0.0, 0.0};
    const rc::math::Vec3 tip = arm.tool(rc::span<const double>(straight.data(), straight.size()))
                                   .translation;

    ++attempted;
    const auto solutions = solve(tip.x, tip.y, kL1, kL2);
    if (!solutions.has_value()) continue;
    if (std::isnan(solutions.value().elbow_up.q1)) continue;
    if (std::isnan(solutions.value().elbow_up.q2)) continue;
    ++solved;
  }

  std::cout << "  fully extended at " << attempted << " angles, solved " << solved << "\n";
  RC_CHECK_EQ(solved, attempted);
}

RC_TEST("no answer is ever not a number") {
  // Anywhere in the workspace, including both boundaries.
  for (int i = -20; i <= 20; ++i) {
    for (int j = -20; j <= 20; ++j) {
      const double x = static_cast<double>(i) * 0.04;
      const double y = static_cast<double>(j) * 0.04;
      const auto solutions = solve(x, y, kL1, kL2);
      if (!solutions.has_value()) continue;

      RC_REQUIRE(std::isfinite(solutions.value().elbow_up.q1));
      RC_REQUIRE(std::isfinite(solutions.value().elbow_up.q2));
      RC_REQUIRE(std::isfinite(solutions.value().elbow_down.q1));
      RC_REQUIRE(std::isfinite(solutions.value().elbow_down.q2));
    }
  }
}

RC_TEST("the shoulder angle is right in every quadrant") {
  // The check that catches a division where an atan2 belongs. A ratio loses the
  // quadrant, so a target behind the arm is solved as though it were in front.
  const rc::kin::Chain arm = planar_arm();
  const double points[][2] = {{0.4, 0.4}, {-0.4, 0.4}, {-0.4, -0.4}, {0.4, -0.4}};

  for (const auto& p : points) {
    const auto solutions = solve(p[0], p[1], kL1, kL2);
    RC_REQUIRE(solutions.has_value());

    const rc::math::Vec3 tool = reached(arm, solutions.value().elbow_up);
    RC_CHECK_NEAR(tool.x, p[0], 1e-12);
    RC_CHECK_NEAR(tool.y, p[1], 1e-12);
  }
}

RC_TEST("near the boundary a small move of the target is a large move of the joints") {
  // Not a defect, and worth seeing: this is what a singularity is. The solver is
  // correct at both points and the machine has to travel a long way between
  // them, which is a planning problem rather than a solver problem.
  const auto inner = solve(0.79, 0.0, kL1, kL2);
  const auto outer = solve(0.7999, 0.0, kL1, kL2);
  RC_REQUIRE(inner.has_value());
  RC_REQUIRE(outer.has_value());

  const double target_moved = 0.7999 - 0.79;
  const double elbow_moved = std::fabs(inner.value().elbow_up.q2 - outer.value().elbow_up.q2);

  std::cout << "  the target moved " << std::fixed << std::setprecision(4) << target_moved
            << " m and the elbow moved " << elbow_moved << " rad\n";
  RC_CHECK(elbow_moved > target_moved * 10.0);
}
