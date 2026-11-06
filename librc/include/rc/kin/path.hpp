// rc/kin/path.hpp
//
// Two ways to move an arm, and what each costs, from lesson 13-09, graduated.
//
// A move in joint space sends each joint from where it is to where it is going in a straight line in
// angle. It cannot fail and it has the lowest peak joint speed there is, and the tool follows a curve.
// A move in tool space keeps the tool on a straight line and solves the joints at each point. It is
// straight, and it can fail, and it costs joint speed.
//
// Numbers from that lesson, a 0.5 m and 0.4 m arm. A move in the joints strayed from the line between its
// ends by 22.7 mm on a move of 0.32 m and 121.1 mm on one of 0.8 m. The straight line between (0.30, 0.05)
// and (-0.30, 0.05), both reachable, entered the hole round the base at point 72 of 200. And moving the tool
// radially out to full reach in two seconds needed 3.39 rad/s of a joint against 0.82 for the joint move,
// 4.2 times as much.

#ifndef RC_KIN_PATH_HPP
#define RC_KIN_PATH_HPP

#include <cmath>
#include <cstddef>
#include <vector>

#include <rc/core/compat.hpp>
#include <rc/kin/planar_arm.hpp>

namespace rc {
namespace kin {

// Where the joints are.
struct JointPose {
  double q1 = 0.0;
  double q2 = 0.0;
};

// Which of the two answers to hold throughout a move. Choosing it once, and keeping it,
// is what keeps a path continuous: a solver that picks whichever branch it likes at each
// point can flip the elbow between two neighbouring points, which the arm cannot do.
enum class Branch { ElbowDown, ElbowUp };

// A move done in joint space: each joint goes from where it is to where it is going, in a
// straight line in angle. t is zero at the start and one at the end.
//
// It is the easy move. There is no inverse kinematics in it and no way for it to fail, and
// both joints arrive together. What it does not do is keep the tool on a line, because the
// tool is a trigonometric function of the angles.
inline JointPose lerp_joints(const JointPose& a, const JointPose& b, double t) {
  JointPose p;
  p.q1 = a.q1 + (b.q1 - a.q1) * t;
  p.q2 = a.q2 + (b.q2 - a.q2) * t;
  return p;
}

// A move done in tool space: the tool goes in a straight line, and at each of `steps + 1`
// points along it the joint angles are whatever put it there.
struct CartesianPath {
  std::vector<JointPose> poses;
  bool complete = true;            // false if the straight line leaves the workspace
  std::size_t first_unreachable = 0;
};

inline CartesianPath cartesian_path(double l1, double l2, double x0, double y0, double x1, double y1,
                                    std::size_t steps, Branch branch) {
  CartesianPath path;
  for (std::size_t i = 0; i <= steps; ++i) {
    const double t = steps == 0 ? 0.0 : static_cast<double>(i) / static_cast<double>(steps);
    const auto solved = solve(x0 + (x1 - x0) * t, y0 + (y1 - y0) * t, l1, l2);

    // The line left the workspace here. Stopping is the honest answer: the points after it
    // may be reachable again and the arm cannot get from one side to the other.
    if (!solved) {
      path.complete = false;
      path.first_unreachable = i;
      return path;
    }

    const ArmSolution& s = branch == Branch::ElbowDown ? solved.value().elbow_down : solved.value().elbow_up;
    path.poses.push_back(JointPose{s.q1, s.q2});
  }
  return path;
}

// The farthest the tool strays from the straight line through two points, over a path of
// joint poses, in metres.
inline double path_deviation(double l1, double l2, double x0, double y0, double x1, double y1,
                             rc::span<const JointPose> path) {
  const double length = std::hypot(x1 - x0, y1 - y0);
  if (length == 0.0) return 0.0;

  // The unit normal to the line, so that the distance of a point from it is one dot product.
  const double nx = -(y1 - y0) / length;
  const double ny = (x1 - x0) / length;

  double worst = 0.0;
  for (const JointPose& p : path) {
    const double x = l1 * std::cos(p.q1) + l2 * std::cos(p.q1 + p.q2);
    const double y = l1 * std::sin(p.q1) + l2 * std::sin(p.q1 + p.q2);
    const double d = std::fabs((x - x0) * nx + (y - y0) * ny);
    if (d > worst) worst = d;
  }
  return worst;
}

// The fastest any joint has to turn, in radians a second, to follow a path in the given
// time, with the poses evenly spread in time.
//
// The turn between two poses is the short way round: an angle is a direction, and a joint
// that goes from 3.1 to -3.1 has moved 0.08, not 6.2.
inline double peak_joint_speed(rc::span<const JointPose> path, double duration) {
  if (path.size() < 2 || duration <= 0.0) return 0.0;

  const double dt = duration / static_cast<double>(path.size() - 1);
  double fastest = 0.0;
  for (std::size_t i = 1; i < path.size(); ++i) {
    const double d1 = std::atan2(std::sin(path[i].q1 - path[i - 1].q1), std::cos(path[i].q1 - path[i - 1].q1));
    const double d2 = std::atan2(std::sin(path[i].q2 - path[i - 1].q2), std::cos(path[i].q2 - path[i - 1].q2));
    const double speed = std::fmax(std::fabs(d1), std::fabs(d2)) / dt;
    if (speed > fastest) fastest = speed;
  }
  return fastest;
}

}  // namespace kin
}  // namespace rc

#endif  // RC_KIN_PATH_HPP
