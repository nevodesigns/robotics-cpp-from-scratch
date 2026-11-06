#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cmath>
#include <limits>

// A planar two link arm in a vertical plane, with mass.
//
// Lessons 13-01 to 13-05 treated the arm as geometry: where the tool is, what
// angles put it there, which of them it can hold. None of that has weight. A real
// arm is held up by its motors, and how hard they must push depends on where the
// arm is, which is the subject of this lesson.
//
// Angles are measured from the horizontal, as in rc/kin/planar_arm.hpp, with the
// elbow angle relative to the first link. Gravity points down the page, so an
// arm lying out along +x is the hardest pose to hold and one standing straight up
// holds itself.
struct ArmBody {
  double l1 = 0.5;        // lengths, metres
  double l2 = 0.4;
  double m1 = 2.0;        // masses, kilograms
  double m2 = 1.2;
  double c1 = 0.25;       // how far from its own joint each link's centre of mass is, metres
  double c2 = 0.20;
  double gravity = 9.81;
};

struct JointTorques {
  double shoulder = 0.0;  // newton metres the motor must supply, positive pushing the arm up
  double elbow = 0.0;
};

// The torque each motor must supply to hold the arm still, with a payload in the
// gripper.
//
// Each link's weight acts at its centre of mass, and a joint feels it through the
// horizontal distance from the joint to that point. So the torque is the weight
// times the cosine of the angle the link makes with the horizontal, and the
// shoulder carries everything the elbow does plus the first link and the whole
// of the second link's weight at the elbow.
inline JointTorques holding_torques(const ArmBody& a, double q1, double q2, double payload) {
  const double q12 = q1 + q2;

  // What hangs beyond the elbow: the second link at its centre of mass, and the
  // payload at the tool.
  const double beyond_elbow = (a.m2 * a.c2 + payload * a.l2) * a.gravity * std::cos(q12);

  JointTorques t;
  t.elbow = beyond_elbow;
  t.shoulder = (a.m1 * a.c1 + (a.m2 + payload) * a.l1) * a.gravity * std::cos(q1) + beyond_elbow;
  return t;
}

// How heavy a payload one joint can hold, given the torque it needs with none, the
// extra it needs for each kilogram, and what its motor can supply.
inline double payload_one_joint_can_hold(double alone, double per_kg, double limit) {
  // It cannot hold itself.
  if (std::fabs(alone) > limit) return 0.0;

  // The payload does nothing to this joint, so it never runs out.
  if (std::fabs(per_kg) < 1e-9) return std::numeric_limits<double>::infinity();

  // The torque alone + per_kg * p reaches the limit at the end of the range of
  // payloads that stay inside it. Which end depends on which way it is heading, and
  // it is heading away from zero.
  return per_kg > 0.0 ? (limit - alone) / per_kg : (-limit - alone) / per_kg;
}

// The heaviest payload the motors can hold in this pose, given what each can
// supply.
//
// The torque is a straight line in the payload, a plus b times p, so the heaviest
// payload each joint can hold is solved for rather than searched for. The answer
// is the smaller of the two. It is zero if the arm cannot hold even itself, and
// infinite if neither joint feels the payload at all, which is the arm standing
// straight up.
inline double heaviest_payload(const ArmBody& a, double q1, double q2, double shoulder_limit,
                               double elbow_limit) {
  const double q12 = q1 + q2;
  const double g = a.gravity;

  const double elbow_alone = a.m2 * a.c2 * g * std::cos(q12);
  const double elbow_per_kg = a.l2 * g * std::cos(q12);
  const double shoulder_alone = (a.m1 * a.c1 + a.m2 * a.l1) * g * std::cos(q1) + elbow_alone;
  const double shoulder_per_kg = (a.l1 * std::cos(q1) + a.l2 * std::cos(q12)) * g;

  const double s = payload_one_joint_can_hold(shoulder_alone, shoulder_per_kg, shoulder_limit);
  const double e = payload_one_joint_can_hold(elbow_alone, elbow_per_kg, elbow_limit);
  return s < e ? s : e;
}

#endif  // LESSON_SOLUTION_HPP
