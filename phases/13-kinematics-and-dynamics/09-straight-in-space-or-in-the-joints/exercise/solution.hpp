#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cmath>
#include <cstddef>
#include <vector>

#include <rc/core/compat.hpp>
#include <rc/kin/planar_arm.hpp>

// Where the joints are.
struct JointPose {
  double q1 = 0.0;
  double q2 = 0.0;
};

// Which of the two answers to hold throughout a move. Choosing it once and keeping it is
// what keeps a path continuous.
enum class Branch { ElbowDown, ElbowUp };

// A move done in joint space: each joint goes from where it is to where it is going in a
// straight line in angle. t is zero at the start and one at the end.
inline JointPose lerp_joints(const JointPose& a, const JointPose& b, double t) {
  // TODO: a plus (b minus a) times t, for each joint.
  (void)a;
  (void)b;
  (void)t;
  return JointPose{};
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
  // TODO: for i from 0 to steps, the point a fraction i / steps of the way from (x0, y0) to
  // (x1, y1). Ask rc::kin::solve(x, y, l1, l2) for the joint angles. It returns an expected:
  // test it, and if it is an error the line has left the workspace, so set complete to false,
  // set first_unreachable to i, and STOP: the points after it may be reachable again and the
  // arm cannot get from one side to the other.
  //
  // Otherwise take the same branch every time, solved.value().elbow_down or .elbow_up: the
  // whole point is that it is chosen once and kept. A solver that picks either at each point
  // can flip the elbow between neighbours, which the arm cannot do.
  (void)l1;
  (void)l2;
  (void)x0;
  (void)y0;
  (void)x1;
  (void)y1;
  (void)steps;
  (void)branch;
  return CartesianPath{};
}

// The farthest the tool strays from the straight line through two points, over a path of
// joint poses, in metres.
inline double path_deviation(double l1, double l2, double x0, double y0, double x1, double y1,
                             rc::span<const JointPose> path) {
  // TODO: the tool at each pose is (l1 cos q1 + l2 cos(q1 + q2), l1 sin q1 + l2 sin(q1 + q2)).
  // The distance of a point from the line is a dot product with the unit normal to it:
  // (-(y1 - y0), x1 - x0) over the length. The answer is the largest absolute distance. A
  // line of no length has no deviation.
  (void)l1;
  (void)l2;
  (void)x0;
  (void)y0;
  (void)x1;
  (void)y1;
  (void)path;
  return 0.0;
}

// The fastest any joint has to turn, in radians a second, to follow a path in the given time,
// with the poses evenly spread in time.
inline double peak_joint_speed(rc::span<const JointPose> path, double duration) {
  // TODO: dt is the duration over the number of gaps between poses. For each pair of
  // neighbours, how far each joint turned divided by dt, and the answer is the largest.
  //
  // How far a joint turned is the SHORT way round: atan2(sin(d), cos(d)) of the difference
  // d. A joint that goes from 3.1 to -3.1 has moved 0.08 and not 6.2. Fewer than two poses,
  // or no time, is no speed.
  (void)path;
  (void)duration;
  return 0.0;
}

#endif  // LESSON_SOLUTION_HPP
