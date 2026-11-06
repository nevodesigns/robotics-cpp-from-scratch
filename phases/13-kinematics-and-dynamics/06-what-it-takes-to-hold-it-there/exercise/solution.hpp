#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cmath>
#include <limits>

// A planar two link arm in a vertical plane, with mass.
//
// Angles are measured from the horizontal, as in rc/kin/planar_arm.hpp, with the
// elbow angle relative to the first link. Gravity points down the page, so an arm
// lying out along +x is the hardest pose to hold and one standing straight up
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
inline JointTorques holding_torques(const ArmBody& a, double q1, double q2, double payload) {
  // TODO: each weight acts at its centre of mass, and a joint feels it through
  // the horizontal distance to that point, which is the weight times the cosine of
  // the angle the link makes with the horizontal.
  //
  // The elbow carries what is beyond it: the second link at its centre of mass,
  // c2 from the elbow, and the payload at the tool, l2 from it. Both act at the
  // angle q1 + q2.
  //
  // The shoulder carries all of that, plus the first link at c1, plus the whole
  // weight of the second link and the payload hanging at the elbow, l1 out, at
  // the angle q1.
  (void)a;
  (void)q1;
  (void)q2;
  (void)payload;
  return JointTorques{};
}

// The heaviest payload the motors can hold in this pose, given what each can
// supply.
inline double heaviest_payload(const ArmBody& a, double q1, double q2, double shoulder_limit,
                               double elbow_limit) {
  // TODO: each torque is a straight line in the payload p, alone + per_kg * p, so
  // the heaviest payload a joint can hold is solved for, not searched for.
  //
  //   - if the arm cannot hold ITSELF, |alone| above the limit, the answer is zero
  //   - if per_kg is zero, the payload does nothing to that joint and it never
  //     runs out: infinity. Compare with a small tolerance, not exactly zero,
  //     because cos(pi / 2) is 6e-17 and not zero
  //   - otherwise the torque reaches the limit at p = (limit - alone) / per_kg when
  //     per_kg is positive and (-limit - alone) / per_kg when it is negative,
  //     because the torque heads away from zero
  //
  // The answer is the smaller of the two joints.
  (void)a;
  (void)q1;
  (void)q2;
  (void)shoulder_limit;
  (void)elbow_limit;
  (void)std::numeric_limits<double>::infinity;
  return 0.0;
}

#endif  // LESSON_SOLUTION_HPP
