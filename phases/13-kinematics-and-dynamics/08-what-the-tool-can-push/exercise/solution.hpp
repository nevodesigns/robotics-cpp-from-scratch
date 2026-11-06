#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cmath>
#include <limits>

#include <rc/kin/singularity.hpp>

using rc::kin::planar_jacobian;
using rc::kin::PlanarJacobian;

// A force the tool pushes on the world with, in newtons, along each axis.
struct ToolForce {
  double fx = 0.0;
  double fy = 0.0;
};

// What each joint must supply, in newton metres.
struct JointLoad {
  double shoulder = 0.0;
  double elbow = 0.0;
};

// The torque at each joint that holds a force at the tool.
inline JointLoad torques_for_force(const PlanarJacobian& j, const ToolForce& force) {
  // TODO: the Jacobian turned over. Power is the same at both ends of a rigid arm, so
  // torque dotted with joint speed equals force dotted with tool speed, and since tool
  // speed is J times joint speed, torque is the TRANSPOSE of J times force.
  //
  // J is [a b; c d], so its transpose is [a c; b d]:
  //   shoulder = a * fx + c * fy
  //   elbow    = b * fx + d * fy
  (void)j;
  (void)force;
  return JointLoad{};
}

// The force at the tool that a pair of joint torques amounts to, if there is one.
inline ToolForce force_for_torques(const PlanarJacobian& j, double shoulder, double elbow, bool& ok) {
  // TODO: the inverse of the transpose. ok is false, and the force zero, where the
  // determinant is about zero (at most 1e-9): there the question has no answer, because
  // a force along the arm puts no torque on either joint.
  //
  // Otherwise, with det = j.determinant():
  //   fx = (d * shoulder - c * elbow) / det
  //   fy = (-b * shoulder + a * elbow) / det
  (void)j;
  (void)shoulder;
  (void)elbow;
  ok = false;
  return ToolForce{};
}

// The hardest the tool can push in a direction before a joint runs out of torque.
inline double strongest_push(const PlanarJacobian& j, double direction_x, double direction_y,
                             double shoulder_limit, double elbow_limit) {
  // TODO: normalise the direction to a unit force (a zero direction pushes nothing: return
  // zero). A unit of force costs a torque at each joint, from torques_for_force. The push
  // can be scaled until the first joint reaches its limit: limit divided by the size of
  // that joint's torque, for each joint, and the answer is the smaller.
  //
  // A joint whose torque per newton is below 1e-12 does not feel the push at all, and
  // never runs out: infinity. A direction that neither feels is carried by the arm's own
  // structure and has no limit.
  (void)j;
  (void)direction_x;
  (void)direction_y;
  (void)shoulder_limit;
  (void)elbow_limit;
  (void)std::numeric_limits<double>::infinity;
  return 0.0;
}

#endif  // LESSON_SOLUTION_HPP
