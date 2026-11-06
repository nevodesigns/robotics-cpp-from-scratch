#include <rc/test/rc_test.hpp>

#include <cmath>
#include <iomanip>
#include <iostream>
#include <vector>

#include "solution.hpp"

namespace {

const double kL1 = 0.5;
const double kL2 = 0.4;
const std::size_t kSteps = 200;
const double kDuration = 2.0;     // seconds for every move

struct Point {
  double x;
  double y;
};

struct Move {
  const char* name;
  Point from;
  Point to;
};

// Where the tool is, from the geometry.
Point tool(const JointPose& p) {
  return Point{kL1 * std::cos(p.q1) + kL2 * std::cos(p.q1 + p.q2), kL1 * std::sin(p.q1) + kL2 * std::sin(p.q1 + p.q2)};
}

// The joint space move between the two ends of a straight line, sampled the same number of
// times as the tool space one, so that the two are compared like for like.
std::vector<JointPose> joint_move(const Move& m, bool& reachable) {
  const auto a = rc::kin::solve(m.from.x, m.from.y, kL1, kL2);
  const auto b = rc::kin::solve(m.to.x, m.to.y, kL1, kL2);
  std::vector<JointPose> path;
  reachable = static_cast<bool>(a) && static_cast<bool>(b);
  if (!reachable) return path;

  const JointPose start{a.value().elbow_down.q1, a.value().elbow_down.q2};
  const JointPose end{b.value().elbow_down.q1, b.value().elbow_down.q2};
  for (std::size_t i = 0; i <= kSteps; ++i) path.push_back(lerp_joints(start, end, static_cast<double>(i) / kSteps));
  return path;
}

const Move kMoves[] = {
    {"short, near the middle", {0.55, 0.20}, {0.35, 0.45}},
    {"across the workspace", {0.70, -0.30}, {0.70, 0.40}},
    {"near full reach", {0.82, -0.25}, {0.82, 0.25}},
    {"through the middle", {0.60, -0.40}, {0.60, 0.40}},
    {"radially out to full reach", {0.55, 0.0}, {0.895, 0.0}},
};

}  // namespace

RC_TEST("a move in joint space starts where it started, ends where it ends, and goes evenly between") {
  const JointPose a{0.2, 1.1};
  const JointPose b{-0.6, 2.0};

  RC_CHECK_NEAR(lerp_joints(a, b, 0.0).q1, 0.2, 1e-15);
  RC_CHECK_NEAR(lerp_joints(a, b, 0.0).q2, 1.1, 1e-15);
  RC_CHECK_NEAR(lerp_joints(a, b, 1.0).q1, -0.6, 1e-15);
  RC_CHECK_NEAR(lerp_joints(a, b, 1.0).q2, 2.0, 1e-15);
  RC_CHECK_NEAR(lerp_joints(a, b, 0.5).q1, -0.2, 1e-15);
  RC_CHECK_NEAR(lerp_joints(a, b, 0.25).q2, 1.1 + 0.9 * 0.25, 1e-15);
}

RC_TEST("a move in tool space puts the tool on the line at every point, and keeps its branch") {
  for (const Move& m : kMoves) {
    const CartesianPath path = cartesian_path(kL1, kL2, m.from.x, m.from.y, m.to.x, m.to.y, kSteps, Branch::ElbowDown);
    RC_REQUIRE(path.complete);
    RC_REQUIRE_EQ(path.poses.size(), kSteps + 1);

    // The ends are the ends, and every point in between is on the line.
    const Point first = tool(path.poses.front());
    const Point last = tool(path.poses.back());
    RC_CHECK_NEAR(first.x, m.from.x, 1e-9);
    RC_CHECK_NEAR(first.y, m.from.y, 1e-9);
    RC_CHECK_NEAR(last.x, m.to.x, 1e-9);
    RC_CHECK_NEAR(last.y, m.to.y, 1e-9);
    RC_CHECK(path_deviation(kL1, kL2, m.from.x, m.from.y, m.to.x, m.to.y,
                            rc::span<const JointPose>(path.poses.data(), path.poses.size())) < 1e-9);

    // One branch throughout: the elbow never changes sign, and no joint jumps between
    // neighbours.
    for (std::size_t i = 0; i < path.poses.size(); ++i) {
      RC_CHECK(path.poses[i].q2 >= 0.0);
      if (i > 0) RC_CHECK(std::fabs(path.poses[i].q1 - path.poses[i - 1].q1) < 0.3);
    }
  }

  const CartesianPath up = cartesian_path(kL1, kL2, 0.7, -0.3, 0.7, 0.4, kSteps, Branch::ElbowUp);
  RC_REQUIRE(up.complete);
  for (const JointPose& p : up.poses) RC_CHECK(p.q2 <= 0.0);
}

RC_TEST("the line between two reachable points can leave the workspace, and the path says where") {
  // Both ends are reachable. The line between them passes 0.05 m from the base, which is
  // inside the hole that the shorter link cannot reach into.
  const CartesianPath path = cartesian_path(kL1, kL2, 0.3, 0.05, -0.3, 0.05, kSteps, Branch::ElbowDown);

  // The line is inside the hole where |x| is under 0.0866, which is between 35.6 and 64.4
  // percent of the way along it.
  std::cout << "  a straight line from (0.30, 0.05) to (-0.30, 0.05), both reachable: it enters the hole round\n"
            << "  the base at point " << path.first_unreachable << " of " << kSteps << ", and the path stops there.\n\n";

  RC_CHECK(!path.complete);
  RC_CHECK(path.first_unreachable >= 70 && path.first_unreachable <= 74);
  RC_CHECK_EQ(path.poses.size(), path.first_unreachable);   // everything before it, and nothing after

  // A move in the joints between the same two ends exists: it simply does not go straight.
  const Move m{"over the base", {0.3, 0.05}, {-0.3, 0.05}};
  bool reachable = false;
  const std::vector<JointPose> joints = joint_move(m, reachable);
  RC_REQUIRE(reachable);
  RC_CHECK(path_deviation(kL1, kL2, 0.3, 0.05, -0.3, 0.05,
                          rc::span<const JointPose>(joints.data(), joints.size())) > 0.05);
}

RC_TEST("moving the joints in a straight line moves the tool in a curve, by centimetres") {
  std::cout << "  five moves of two seconds, each joint going from where it is to where it is going in a straight\n"
            << "  line in angle. How far the tool strays from the straight line between the ends:\n\n"
            << "    move                           length    strays\n";

  double worst = 0.0;
  for (const Move& m : kMoves) {
    bool reachable = false;
    const std::vector<JointPose> path = joint_move(m, reachable);
    RC_REQUIRE(reachable);
    const double length = std::hypot(m.to.x - m.from.x, m.to.y - m.from.y);
    const double strays = path_deviation(kL1, kL2, m.from.x, m.from.y, m.to.x, m.to.y,
                                         rc::span<const JointPose>(path.data(), path.size()));
    std::cout << "    " << std::left << std::setw(28) << m.name << std::right << std::fixed << std::setprecision(3)
              << std::setw(8) << length << " m" << std::setprecision(1) << std::setw(8) << strays * 1000.0 << " mm\n";

    // Every one of them is off the line by more than a centimetre, and the end to end
    // sweeps by a good deal more.
    RC_CHECK(strays > 0.010);
    if (strays > worst) worst = strays;
  }
  std::cout << "\n  The worst is " << std::setprecision(0) << worst * 1000.0
            << " mm, on a move of 0.8 m: a seventh of its length.\n\n";

  RC_CHECK(worst > 0.1);
}

RC_TEST("keeping the tool straight costs joint speed, and a great deal more near full reach") {
  std::cout << "  the fastest any joint has to turn to make the same move in two seconds:\n\n"
            << "    move                           joints straight   tool straight   ratio\n"
            << "                                          (rad/s)         (rad/s)\n";

  double ratio_radial = 0.0;
  for (const Move& m : kMoves) {
    bool reachable = false;
    const std::vector<JointPose> joints = joint_move(m, reachable);
    RC_REQUIRE(reachable);
    const CartesianPath straight = cartesian_path(kL1, kL2, m.from.x, m.from.y, m.to.x, m.to.y, kSteps, Branch::ElbowDown);
    RC_REQUIRE(straight.complete);

    const double slow = peak_joint_speed(rc::span<const JointPose>(joints.data(), joints.size()), kDuration);
    const double fast = peak_joint_speed(rc::span<const JointPose>(straight.poses.data(), straight.poses.size()), kDuration);
    std::cout << "    " << std::left << std::setw(28) << m.name << std::right << std::fixed << std::setprecision(2)
              << std::setw(14) << slow << std::setw(16) << fast << std::setprecision(1) << std::setw(9) << fast / slow
              << "\n";

    // The joint move has the least peak speed there is for those ends, since it is a constant
    // rate, and the tool move needs at least as much.
    RC_CHECK(fast >= slow * 0.99);
    if (std::string(m.name) == "radially out to full reach") ratio_radial = fast / slow;
  }
  std::cout << "\n  Moving the tool in a straight line towards full extension is moving into the singularity of lesson\n"
            << "  13-04: the tool goes at an even speed and the elbow has to turn faster and faster to keep up.\n\n";

  RC_CHECK(ratio_radial > 3.0);
}

RC_TEST("a joint that goes from 3.1 to -3.1 has turned 0.08 radians, and not 6.2") {
  std::vector<JointPose> path;
  path.push_back(JointPose{3.1, 0.0});
  path.push_back(JointPose{-3.1, 0.0});

  // 0.083 radians in one second.
  RC_CHECK_NEAR(peak_joint_speed(rc::span<const JointPose>(path.data(), path.size()), 1.0), 6.283185307179586 - 6.2, 1e-9);

  RC_CHECK_NEAR(peak_joint_speed(rc::span<const JointPose>(), 1.0), 0.0, 1e-15);
  RC_CHECK_NEAR(peak_joint_speed(rc::span<const JointPose>(path.data(), 1), 1.0), 0.0, 1e-15);
  RC_CHECK_NEAR(peak_joint_speed(rc::span<const JointPose>(path.data(), path.size()), 0.0), 0.0, 1e-15);
}
