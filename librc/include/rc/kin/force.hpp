// rc/kin/force.hpp
//
// What the tool can push, from lesson 13-08, graduated. It builds on the Jacobian of
// rc/kin/singularity.hpp.
//
// The Jacobian maps joint speeds to tool speed. Turned over, the same matrix maps tool
// force to joint torque: power is the same at both ends of a rigid arm. A force is held
// by torques of J transposed times the force, and the force a pair of torques amounts to
// is the inverse of that, which does not exist at full extension.
//
// Numbers from that lesson, motors of 20 and 8 N m on a 0.5 m and 0.4 m arm. With the elbow
// at 0.05 rad the tool can push 720.1 N along the arm and 20.0 N across it, and move 0.02 m/s
// along it and 1.80 m/s across: 36 times stronger along and 81 times slower. The direction
// the arm cannot move in is the direction it can push hardest. And estimating a force from
// joint torques near a straight arm is hopeless: at 0.01 rad, 0.1 N m of torque changes the
// estimated force by 20.0 N.

#ifndef RC_KIN_FORCE_HPP
#define RC_KIN_FORCE_HPP

#include <cmath>
#include <limits>

#include <rc/kin/singularity.hpp>

namespace rc {
namespace kin {

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
//
// Lesson 13-04's Jacobian maps joint speeds to tool speed. The same matrix, turned over,
// maps tool force to joint torque. Power is the same at both ends of a rigid arm, so the
// torques dotted with the joint speeds is the force dotted with the tool speed, and since
// tool speed is J times joint speed, that makes torque the transpose of J times force.
inline JointLoad torques_for_force(const PlanarJacobian& j, const ToolForce& force) {
  JointLoad load;
  load.shoulder = j.a * force.fx + j.c * force.fy;
  load.elbow = j.b * force.fx + j.d * force.fy;
  return load;
}

// The force at the tool that a pair of joint torques amounts to, if there is one.
//
// It is the inverse of the transpose, so it needs the determinant, and where that is zero
// the question has no answer. At full extension the force along the arm puts no torque on
// either joint, so a torque sensor in each joint cannot see it at all, and `ok` says so.
inline ToolForce force_for_torques(const PlanarJacobian& j, double shoulder, double elbow, bool& ok) {
  const double det = j.determinant();
  ToolForce force;
  ok = std::fabs(det) > 1e-9;
  if (!ok) return force;

  force.fx = (j.d * shoulder - j.c * elbow) / det;
  force.fy = (-j.b * shoulder + j.a * elbow) / det;
  return force;
}

// The hardest the tool can push in a direction before a joint runs out of torque.
//
// A unit of force in that direction costs a torque at each joint, and the joints have a
// limit each, so the push can be scaled until the first of them reaches its limit. Where
// a direction costs no torque at either joint, the arm's own structure carries it and
// there is no limit: infinity.
inline double strongest_push(const PlanarJacobian& j, double direction_x, double direction_y,
                             double shoulder_limit, double elbow_limit) {
  const double length = std::hypot(direction_x, direction_y);
  if (length == 0.0) return 0.0;

  ToolForce unit;
  unit.fx = direction_x / length;
  unit.fy = direction_y / length;
  const JointLoad per_newton = torques_for_force(j, unit);

  const double s = std::fabs(per_newton.shoulder);
  const double e = std::fabs(per_newton.elbow);
  const double inf = std::numeric_limits<double>::infinity();

  // 1e-12 of a newton metre per newton is the arm carrying it, not a lever.
  const double by_shoulder = s > 1e-12 ? shoulder_limit / s : inf;
  const double by_elbow = e > 1e-12 ? elbow_limit / e : inf;
  return by_shoulder < by_elbow ? by_shoulder : by_elbow;
}

}  // namespace kin
}  // namespace rc

#endif  // RC_KIN_FORCE_HPP
