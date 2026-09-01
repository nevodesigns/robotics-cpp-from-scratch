#include <rc/test/rc_test.hpp>

#include <rc/kin/chain.hpp>
#include <rc/kin/planar_arm.hpp>
#include <rc/math/angles.hpp>

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <vector>

#include "solution.hpp"

namespace {

using rc::math::Vec3;

constexpr double kL1 = 0.5;
constexpr double kL2 = 0.3;

rc::kin::Chain planar_arm() {
  rc::kin::Chain arm;
  const Vec3 z{0.0, 0.0, 1.0};
  arm.add(rc::kin::revolute_along_x(0.0, z));
  arm.add(rc::kin::revolute_along_x(kL1, z));
  arm.add(rc::kin::revolute_along_x(kL2, z));
  return arm;
}

rc::span<const double> view(const std::vector<double>& v) {
  return rc::span<const double>(v.data(), v.size());
}

Vec3 tool_at(const rc::kin::Chain& arm, const std::vector<double>& q) {
  return arm.tool(view(q)).translation;
}

// How far the found angles are from the nearer of the two exact answers.
double distance_to_exact(const std::vector<double>& q, double x, double y) {
  const auto exact = rc::kin::solve(x, y, kL1, kL2);
  if (!exact.has_value()) return -1.0;

  double best = 1e30;
  for (const auto& s : {exact.value().elbow_up, exact.value().elbow_down}) {
    const double d = std::hypot(rc::math::wrap_angle(q[0] - s.q1),
                                rc::math::wrap_angle(q[1] - s.q2));
    best = std::min(best, d);
  }
  return best;
}

}  // namespace

RC_TEST("a column of the Jacobian is how the tool moves when one joint moves") {
  // Checked against the derivative of the closed form, which can be written
  // down without any of this.
  const rc::kin::Chain arm = planar_arm();
  const double q1 = 0.6;
  const double q2 = 0.9;
  const std::vector<double> q{q1, q2, 0.0};

  const std::vector<Vec3> columns = jacobian(arm, view(q), 1e-6);
  RC_REQUIRE_EQ(columns.size(), static_cast<std::size_t>(3));

  RC_CHECK_NEAR(columns[0].x, -kL1 * std::sin(q1) - kL2 * std::sin(q1 + q2), 1e-8);
  RC_CHECK_NEAR(columns[0].y, kL1 * std::cos(q1) + kL2 * std::cos(q1 + q2), 1e-8);
  RC_CHECK_NEAR(columns[1].x, -kL2 * std::sin(q1 + q2), 1e-8);
  RC_CHECK_NEAR(columns[1].y, kL2 * std::cos(q1 + q2), 1e-8);
}

RC_TEST("a joint that cannot move the tool has a column of zeros") {
  // The third joint of this arm carries the tool and does not turn it, so
  // moving it does nothing to where the tool is. That is a rank deficient
  // Jacobian, and it is the ordinary case rather than a pathology.
  const rc::kin::Chain arm = planar_arm();
  const std::vector<double> q{0.4, 0.7, 0.0};
  const std::vector<Vec3> columns = jacobian(arm, view(q), 1e-6);

  RC_CHECK_NEAR(rc::math::length(columns[2]), 0.0, 1e-9);
}

RC_TEST("smaller steps are not more accurate, and here is the curve") {
  // The measurement that decides the default. Truncation error falls as the
  // square of the step, and rounding error rises as the step shrinks, so there
  // is a best step and it is nowhere near the smallest one.
  const rc::kin::Chain arm = planar_arm();
  const double q1 = 0.6;
  const double q2 = 0.9;
  const std::vector<double> q{q1, q2, 0.0};

  const double exact_x = -kL1 * std::sin(q1) - kL2 * std::sin(q1 + q2);
  const double exact_y = kL1 * std::cos(q1) + kL2 * std::cos(q1 + q2);

  const double steps[] = {1e-1, 1e-2, 1e-4, 1e-6, 1e-8, 1e-10, 1e-12, 1e-14};
  double best_error = 1e30;
  double best_step = 0.0;

  std::cout << "\n  finite difference step against the exact derivative\n";
  for (const double step : steps) {
    const std::vector<Vec3> columns = jacobian(arm, view(q), step);
    const double error = std::hypot(columns[0].x - exact_x, columns[0].y - exact_y);
    std::cout << "    step " << std::scientific << std::setprecision(0) << step
              << "   error " << std::setprecision(2) << error << "\n";
    if (error < best_error) { best_error = error; best_step = step; }
  }

  // The largest step and the smallest are both bad, and the best is in between.
  RC_CHECK(best_step > 1e-10);
  RC_CHECK(best_step < 1e-2);
  RC_CHECK(best_error < 1e-9);
}

RC_TEST("the solver reaches an ordinary target") {
  const rc::kin::Chain arm = planar_arm();
  const std::vector<double> start{0.3, 0.3, 0.0};
  const Vec3 target{0.6, 0.2, 0.0};

  const ReachResult result = reach(arm, view(start), target, ReachOptions{});
  RC_REQUIRE(result.converged);
  RC_CHECK(result.error < 1e-9);

  const Vec3 at = tool_at(arm, result.angles);
  RC_CHECK_NEAR(at.x, target.x, 1e-8);
  RC_CHECK_NEAR(at.y, target.y, 1e-8);
}

RC_TEST("the numerical answer is one of the exact answers") {
  // The check worth the most. A solver that converges to something is not the
  // same as a solver that converges to the right thing, and the closed form
  // from lesson 13-02 says what the right things are.
  const rc::kin::Chain arm = planar_arm();
  const double targets[][2] = {{0.6, 0.2}, {0.2, 0.7}, {-0.3, 0.4}, {0.25, 0.0}, {0.4, -0.5}};

  std::cout << "\n  numerical against closed form\n";
  for (const auto& t : targets) {
    const std::vector<double> start{0.3, 0.3, 0.0};
    const ReachResult result = reach(arm, view(start), Vec3{t[0], t[1], 0.0}, ReachOptions{});
    RC_REQUIRE(result.converged);

    const double away = distance_to_exact(result.angles, t[0], t[1]);
    std::cout << "    (" << std::fixed << std::setprecision(2) << t[0] << ", " << t[1]
              << ")  " << std::setw(6) << result.iterations << " iterations, "
              << std::scientific << std::setprecision(1) << away << " rad from an exact answer\n";
    RC_CHECK(away >= 0.0);
    RC_CHECK(away < 1e-6);
  }
}

RC_TEST("an unreachable target stops rather than looping for ever") {
  // The check that catches a solver with no cap. Nothing about the arithmetic
  // notices that a target is outside the workspace: the error simply stops
  // going down, and without a limit the loop never leaves.
  const rc::kin::Chain arm = planar_arm();
  const std::vector<double> start{0.3, 0.3, 0.0};

  ReachOptions options;
  options.max_iterations = 500;

  const ReachResult result = reach(arm, view(start), Vec3{5.0, 0.0, 0.0}, options);
  RC_CHECK(!result.converged);
  RC_CHECK_EQ(result.iterations, 500);
  RC_CHECK(result.error > 1.0);
}

RC_TEST("giving up is reported, not disguised as an answer") {
  const rc::kin::Chain arm = planar_arm();
  const std::vector<double> start{0.3, 0.3, 0.0};

  ReachOptions options;
  options.max_iterations = 5;   // far too few

  const ReachResult result = reach(arm, view(start), Vec3{0.6, 0.2, 0.0}, options);
  RC_REQUIRE(!result.converged);

  // The angles it hands back are real angles that do not reach the target, and
  // the only thing separating them from an answer is the flag.
  const Vec3 at = tool_at(arm, result.angles);
  RC_CHECK(std::hypot(at.x - 0.6, at.y - 0.2) > 1e-6);
}

RC_TEST("near the edge of the workspace it slows down, and then it stalls") {
  // Not a defect. As the arm straightens, the two columns of the Jacobian line
  // up, and the direction that would take the tool further out stops existing.
  // This is the singularity from lesson 13-02, seen from the other side.
  const rc::kin::Chain arm = planar_arm();
  const std::vector<double> start{0.3, 0.3, 0.0};

  ReachOptions options;
  options.max_iterations = 20000;

  std::cout << "\n  approaching the workspace edge at " << kL1 + kL2 << " metres\n";
  int previous = 0;
  for (const double x : {0.60, 0.75, 0.79, 0.799}) {
    const ReachResult result = reach(arm, view(start), Vec3{x, 0.0, 0.0}, options);
    std::cout << "    target " << std::fixed << std::setprecision(3) << x << "   "
              << (result.converged ? std::to_string(result.iterations) + " iterations"
                                   : std::string("gave up"))
              << ", error " << std::scientific << std::setprecision(1) << result.error << "\n";
    if (result.converged) {
      RC_CHECK(result.iterations >= previous);
      previous = result.iterations;
    }
  }

  // The nearest target is easy and the one on the boundary is not reached at
  // all within the budget.
  const ReachResult easy = reach(arm, view(start), Vec3{0.60, 0.0, 0.0}, options);
  const ReachResult hard = reach(arm, view(start), Vec3{0.799, 0.0, 0.0}, options);
  RC_REQUIRE(easy.converged);
  RC_CHECK(!hard.converged);
  RC_CHECK(hard.error < 1e-5);   // and it got very close before stalling
}

RC_TEST("starting from the answer takes no work at all") {
  const rc::kin::Chain arm = planar_arm();
  const auto exact = rc::kin::solve(0.6, 0.2, kL1, kL2);
  RC_REQUIRE(exact.has_value());

  const std::vector<double> start{exact.value().elbow_up.q1, exact.value().elbow_up.q2, 0.0};
  const ReachResult result = reach(arm, view(start), Vec3{0.6, 0.2, 0.0}, ReachOptions{});

  RC_REQUIRE(result.converged);
  RC_CHECK_EQ(result.iterations, 0);
}

RC_TEST("the solver works on an arm that has no closed form at all") {
  // The point of the whole method. This arm is not planar, its joints do not
  // share an axis, and nobody has derived a formula for it.
  rc::kin::Chain arm;
  const Vec3 z{0.0, 0.0, 1.0};
  const Vec3 y{0.0, 1.0, 0.0};
  arm.add(rc::kin::revolute_along_x(0.00, z));
  arm.add(rc::kin::revolute_along_x(0.30, y));
  arm.add(rc::kin::revolute_along_x(0.25, y));
  arm.add(rc::kin::revolute_along_x(0.15, z));
  arm.add(rc::kin::revolute_along_x(0.10, y));

  const std::vector<double> start{0.2, -0.3, 0.5, 0.1, 0.2};
  const Vec3 target{0.35, 0.20, 0.25};

  ReachOptions options;
  options.max_iterations = 20000;
  const ReachResult result = reach(arm, view(start), target, options);

  std::cout << "\n  a five joint arm with no formula: "
            << (result.converged ? "reached in " + std::to_string(result.iterations) +
                                       " iterations"
                                 : "gave up")
            << "\n";

  RC_REQUIRE(result.converged);
  const Vec3 at = tool_at(arm, result.angles);
  RC_CHECK(std::hypot(std::hypot(at.x - target.x, at.y - target.y), at.z - target.z) < 1e-8);
}
