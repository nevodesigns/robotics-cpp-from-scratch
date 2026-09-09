#include <rc/test/rc_test.hpp>

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <vector>

#include "solution.hpp"

namespace {

constexpr double kFirst = 0.5;
constexpr double kSecond = 0.4;

// A shoulder that cannot swing far clockwise and an elbow that cannot fold all
// the way either side. Neither limit is symmetric and neither is generous,
// which is what makes them worth measuring.
ArmLimits arm_limits() {
  ArmLimits limits;
  limits.shoulder = Limit{-0.6, 2.2};
  limits.elbow = Limit{-2.4, 2.4};
  return limits;
}

// The two joint search this lesson replaces, kept here so the header can be
// checked against it rather than trusted.
struct GridResult {
  double q1 = 0.0;
  double q2 = 0.0;
  double distance = 0.0;
  long long evaluations = 0;
};

GridResult grid_search(double x, double y, const ArmLimits& limits, int steps) {
  GridResult best;
  best.distance = -1.0;
  for (int i = 0; i <= steps; ++i) {
    const double q1 =
        limits.shoulder.lo + limits.shoulder.span() * i / static_cast<double>(steps);
    for (int j = 0; j <= steps; ++j) {
      const double q2 =
          limits.elbow.lo + limits.elbow.span() * j / static_cast<double>(steps);
      const ToolPoint p = tool_at(kFirst, kSecond, q1, q2);
      const double d = std::hypot(p.x - x, p.y - y);
      ++best.evaluations;
      if (best.distance < 0.0 || d < best.distance) {
        best.distance = d;
        best.q1 = q1;
        best.q2 = q2;
      }
    }
  }
  return best;
}

struct Target {
  double x;
  double y;
};

}  // namespace

RC_TEST("clamping an angle is not clamping a number") {
  const Limit shoulder{-0.6, 2.2};

  // Every one of these directions is outside the range. A number clamp sends
  // them all to whichever end is numerically nearer. An angle clamp sends them
  // to whichever end is nearer to point at, which is a different question.
  const double wanted[] = {-3.00, -2.60, -2.20, 2.80, 3.10, -6.10};

  std::cout << "\n  the same direction, clamped two ways\n\n";
  std::cout << "  " << std::setw(9) << "wanted" << std::setw(11) << "as number"
            << std::setw(10) << "as angle" << std::setw(13) << "number off"
            << std::setw(12) << "angle off" << "\n";

  int number_worse = 0;
  for (double angle : wanted) {
    const double by_number = clamp_number(shoulder, angle);
    const double by_angle = clamp_angle(shoulder, angle);
    const double number_gap = std::fabs(angle_between(angle, by_number));
    const double angle_gap = std::fabs(angle_between(angle, by_angle));
    if (angle_gap < number_gap - 1e-12) ++number_worse;

    std::cout << "  " << std::fixed << std::setprecision(3) << std::setw(9) << angle
              << std::setw(11) << by_number << std::setw(10) << by_angle
              << std::setw(13) << number_gap << std::setw(12) << angle_gap << "\n";
  }
  std::cout << "\n  the number clamp picked the further end for " << number_worse
            << " of 6\n\n";

  // The angle clamp is never worse, because it answers the question that was
  // asked. On at least one of these it is strictly better, which is the whole
  // reason the distinction is in the header.
  RC_CHECK(number_worse > 0);

  for (double angle : wanted) {
    const double angle_gap = std::fabs(angle_between(angle, clamp_angle(shoulder, angle)));
    const double number_gap = std::fabs(angle_between(angle, clamp_number(shoulder, angle)));
    RC_CHECK(angle_gap <= number_gap + 1e-12);
  }

  // A direction that is legal at some turn is legal, whatever number carries it.
  RC_CHECK(holds(shoulder, 1.0));
  RC_CHECK(holds(shoulder, 1.0 + kTurn));
  RC_CHECK(holds(shoulder, 1.0 - kTurn));
  RC_CHECK(!holds(shoulder, 3.0));
  RC_CHECK_NEAR(clamp_angle(shoulder, 1.0 + kTurn), 1.0, 1e-12);
}

RC_TEST("the limits take a share of the workspace, and take it from the middle") {
  const ArmLimits limits = arm_limits();

  int geometric = 0, legal = 0, both = 0, up_only = 0, down_only = 0;
  for (int ix = -100; ix <= 100; ++ix) {
    for (int iy = -100; iy <= 100; ++iy) {
      const double x = ix * 0.01, y = iy * 0.01;
      const auto solved = legal_solutions(x, y, kFirst, kSecond, limits);
      if (!solved) continue;
      const LegalSolutions& solutions = solved.value();
      ++geometric;
      if (solutions.any()) ++legal;
      if (solutions.count() == 2) ++both;
      else if (solutions.up_holds) ++up_only;
      else if (solutions.down_holds) ++down_only;
    }
  }

  std::cout << "\n  the workspace on a 1 cm grid, " << geometric
            << " cells reachable by geometry alone\n\n";
  const auto row = [&](const char* label, int count) {
    std::cout << "  " << std::left << std::setw(34) << label << std::right
              << std::setw(7) << count << "  " << std::fixed << std::setprecision(1)
              << std::setw(5) << (100.0 * count / geometric) << "%\n";
  };
  row("legal within the joint limits", legal);
  row("both elbow branches legal", both);
  row("elbow-up branch only", up_only);
  row("elbow-down branch only", down_only);

  const int one_branch = both + up_only;
  std::cout << "\n  a solver that only tries elbow-up finds " << one_branch << " of "
            << legal << " legal cells (" << std::setprecision(1)
            << (100.0 * one_branch / legal) << "%)\n\n";

  // The corners of a two metre square are well outside a 0.9 m arm, so a
  // count of every cell tried would mean the geometry check never ran.
  RC_CHECK(geometric > 20000);
  RC_CHECK(geometric < 201 * 201);
  RC_CHECK(legal < geometric);
  RC_CHECK(up_only > 0);
  RC_CHECK(down_only > 0);

  // The number that matters: solving one branch and stopping does not lose a
  // rounding error's worth of the workspace, it loses a slice of it.
  RC_CHECK(one_branch < legal * 0.9);
}

RC_TEST("the other elbow is the answer, far more often than clamping is") {
  const ArmLimits limits = arm_limits();

  const Target targets[] = {{0.574, -0.483}, {0.466, -0.587},
                            {0.340, -0.668}, {0.201, -0.723}};

  std::cout << "\n  targets where at least one elbow is out of reach\n\n";
  std::cout << "  " << std::setw(8) << "x" << std::setw(8) << "y" << std::setw(10)
            << "up" << std::setw(12) << "down" << std::setw(15) << "clamped err"
            << std::setw(16) << "other branch" << "\n";

  int rescued_by_branch = 0;
  int needed_more_than_a_branch = 0;
  for (const Target& target : targets) {
    const auto solved = legal_solutions(target.x, target.y, kFirst, kSecond, limits);
    RC_CHECK(solved.has_value());
    const LegalSolutions& solutions = solved.value();

    // At least one branch is unavailable on every one of these, which is what
    // makes them worth looking at. Take the branch the arm cannot hold as the
    // one an unlucky solver returned, and clamp it the way that solver would.
    RC_CHECK(solutions.count() < 2);
    const rc::kin::ArmSolution& refused =
        solutions.up_holds ? solutions.elbow_down : solutions.elbow_up;

    const double q1 = clamp_angle(limits.shoulder, refused.q1);
    const double q2 = clamp_angle(limits.elbow, refused.q2);
    const ToolPoint clamped = tool_at(kFirst, kSecond, q1, q2);
    const double clamped_error =
        std::hypot(clamped.x - target.x, clamped.y - target.y);

    double branch_error = -1.0;
    if (solutions.any()) {
      const rc::kin::ArmSolution& usable =
          solutions.up_holds ? solutions.elbow_up : solutions.elbow_down;
      const ToolPoint hit = tool_at(kFirst, kSecond, usable.q1, usable.q2);
      branch_error = std::hypot(hit.x - target.x, hit.y - target.y);
      ++rescued_by_branch;
    } else {
      ++needed_more_than_a_branch;
    }

    std::cout << "  " << std::fixed << std::setprecision(3) << std::setw(8) << target.x
              << std::setw(8) << target.y << std::setw(10)
              << (solutions.up_holds ? "yes" : "no") << std::setw(12)
              << (solutions.down_holds ? "yes" : "no") << std::setw(15)
              << std::setprecision(4) << clamped_error;
    if (branch_error < 0.0) {
      std::cout << std::setw(16) << "no branch left";
    } else {
      std::cout << std::setw(16) << branch_error;
    }
    std::cout << "\n";

    if (solutions.any()) {
      // The finding of this lesson in one line: the branch the solver threw
      // away lands on the target exactly, and the clamp it reached for instead
      // is out by more than a tenth of a metre.
      RC_CHECK(branch_error < 1e-12);
      RC_CHECK(clamped_error > 0.1);
    }
  }

  std::cout << "\n  " << rescued_by_branch
            << " of 4 were reachable all along, by the other elbow\n";
  std::cout << "  " << needed_more_than_a_branch
            << " of 4 the arm genuinely cannot reach\n\n";

  RC_CHECK(rescued_by_branch >= 3);
  RC_CHECK(needed_more_than_a_branch >= 1);
}

RC_TEST("the nearest pose the arm can hold, without searching two joints") {
  const ArmLimits limits = arm_limits();

  const Target targets[] = {{0.574, -0.483}, {0.201, -0.723}, {0.400, -0.800},
                            {0.950, 0.300},  {0.050, 0.020},  {-0.600, -0.400},
                            {0.300, 0.900},  {-0.850, 0.100}};

  std::cout << "\n  one joint scanned against two joints gridded\n\n";
  std::cout << "  " << std::setw(8) << "x" << std::setw(8) << "y" << std::setw(13)
            << "grid" << std::setw(13) << "scan" << std::setw(16) << "scan minus"
            << "\n";

  long long grid_work = 0, scan_work = 0;
  double worst_gap = 0.0;
  for (const Target& target : targets) {
    const GridResult grid = grid_search(target.x, target.y, limits, 600);
    const NearestPose scan =
        nearest_reachable(target.x, target.y, kFirst, kSecond, limits, 2000);

    grid_work += grid.evaluations;
    scan_work += scan.evaluations;
    worst_gap = std::max(worst_gap, scan.distance - grid.distance);

    std::cout << "  " << std::fixed << std::setprecision(3) << std::setw(8) << target.x
              << std::setw(8) << target.y << std::setprecision(6) << std::setw(13)
              << grid.distance << std::setw(13) << scan.distance << std::scientific
              << std::setw(16) << (scan.distance - grid.distance) << std::fixed << "\n";

    // Whatever it returns, it must be a pose the arm can actually take up.
    RC_CHECK(holds(limits.shoulder, scan.q1));
    RC_CHECK(holds(limits.elbow, scan.q2));

    // And the distance it reports must be the distance to the pose it
    // returned. A solver that reports a number it did not achieve is worse
    // than one that fails, because nothing downstream can tell.
    const ToolPoint landed = tool_at(kFirst, kSecond, scan.q1, scan.q2);
    RC_CHECK_NEAR(std::hypot(landed.x - target.x, landed.y - target.y), scan.distance,
                  1e-12);
    RC_CHECK_NEAR(landed.x, scan.tool.x, 1e-12);
    RC_CHECK_NEAR(landed.y, scan.tool.y, 1e-12);
    RC_CHECK(scan.evaluations > 0);
  }

  std::cout << "\n  grid evaluations " << grid_work << ", scan evaluations " << scan_work;
  if (scan_work > 0) {
    std::cout << ", ratio " << std::setprecision(0) << (grid_work / scan_work) << "x";
  }
  std::cout << "\n";
  std::cout << "  worst the scan trailed the grid: " << std::scientific
            << std::setprecision(2) << worst_gap << " m\n\n";

  // Solving the shoulder exactly beats snapping it to a grid line, so the
  // cheaper search is not the worse one.
  RC_CHECK(worst_gap < 1e-4);
  RC_CHECK(scan_work * 100 < grid_work);
}

RC_TEST("how fine the elbow scan has to be") {
  const ArmLimits limits = arm_limits();
  const Target targets[] = {{0.574, -0.483}, {0.201, -0.723}, {-0.600, -0.400},
                            {0.950, 0.300},  {0.300, 0.900}};

  std::cout << "\n  " << std::setw(9) << "steps" << std::setw(15) << "worst gap m"
            << "\n";

  std::vector<double> gaps;
  const int step_counts[] = {8, 32, 128, 512, 2048};
  for (int steps : step_counts) {
    double worst = 0.0;
    for (const Target& target : targets) {
      const GridResult grid = grid_search(target.x, target.y, limits, 600);
      const NearestPose scan =
          nearest_reachable(target.x, target.y, kFirst, kSecond, limits, steps);
      worst = std::max(worst, scan.distance - grid.distance);
    }
    gaps.push_back(worst);
    std::cout << "  " << std::setw(9) << steps << std::scientific
              << std::setprecision(2) << std::setw(15) << worst << "\n";
  }
  std::cout << "\n";

  // More steps never cost accuracy, and the coarsest is measurably worse than
  // the finest. A scan resolution is a number to choose, not a constant to
  // inherit from whoever wrote it first.
  for (std::size_t i = 1; i < gaps.size(); ++i) RC_CHECK(gaps[i] <= gaps[i - 1] + 1e-12);
  RC_CHECK(gaps.front() > gaps.back());
}

RC_TEST("a target it can reach gets reached, and one it cannot gets reported") {
  const ArmLimits limits = arm_limits();

  // Somewhere comfortably inside the legal set.
  const ToolPoint easy = tool_at(kFirst, kSecond, 0.8, 0.9);
  const NearestPose on_target =
      nearest_reachable(easy.x, easy.y, kFirst, kSecond, limits, 4096);
  std::cout << "\n  a legal target: missed by " << std::scientific
            << std::setprecision(2) << on_target.distance << " m\n";
  RC_CHECK(on_target.distance < 1e-3);

  // Behind the shoulder stop, where nothing legal comes close.
  const NearestPose far_off = nearest_reachable(-0.6, -0.4, kFirst, kSecond, limits, 4096);
  std::cout << "  behind the stop: nearest legal pose is " << std::fixed
            << std::setprecision(3) << far_off.distance << " m away, at q1 "
            << far_off.q1 << " q2 " << far_off.q2 << "\n\n";

  RC_CHECK(far_off.distance > 0.3);
  RC_CHECK(holds(limits.shoulder, far_off.q1));
  RC_CHECK(holds(limits.elbow, far_off.q2));

  // And it sits on a stop, because that is where the closest legal pose is when
  // the target is on the wrong side of one.
  const bool on_a_stop = std::fabs(far_off.q1 - limits.shoulder.lo) < 1e-9 ||
                         std::fabs(far_off.q1 - limits.shoulder.hi) < 1e-9;
  RC_CHECK(on_a_stop);

  // Geometry still refuses what geometry refuses: the limits are a second
  // filter, not a replacement for the first.
  const auto too_far = legal_solutions(2.0, 0.0, kFirst, kSecond, limits);
  RC_CHECK(!too_far.has_value());
  RC_CHECK(too_far.error() == rc::kin::ReachError::TooFar);
}
