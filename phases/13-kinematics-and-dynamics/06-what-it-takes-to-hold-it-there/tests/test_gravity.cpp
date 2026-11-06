#include <rc/test/rc_test.hpp>

#include <cmath>
#include <iomanip>
#include <iostream>
#include <limits>

#include "solution.hpp"

namespace {

const double kHalfPi = 1.5707963267948966;
const double kPi = 3.141592653589793;
const double kPayload = 0.5;     // kilograms in the gripper
const double kInertia = 0.5;     // a joint's inertia in the simulation, kg m^2

struct Pose {
  double q1 = 0.0;
  double q2 = 0.0;
};

// What gravity does, worked out a different way from the code under test: the
// potential energy of every mass, differentiated numerically. The torque needed to
// hold a pose still is the slope of the energy with respect to each joint, so this
// shares no algebra with the formula in the solution.
double energy(const ArmBody& a, double q1, double q2, double payload) {
  const double q12 = q1 + q2;
  return a.gravity * (a.m1 * a.c1 * std::sin(q1) + a.m2 * (a.l1 * std::sin(q1) + a.c2 * std::sin(q12)) +
                      payload * (a.l1 * std::sin(q1) + a.l2 * std::sin(q12)));
}

JointTorques slope_of_energy(const ArmBody& a, double q1, double q2, double payload) {
  const double h = 1e-6;
  JointTorques t;
  t.shoulder = (energy(a, q1 + h, q2, payload) - energy(a, q1 - h, q2, payload)) / (2.0 * h);
  t.elbow = (energy(a, q1, q2 + h, payload) - energy(a, q1, q2 - h, payload)) / (2.0 * h);
  return t;
}

double tool_distance(const ArmBody& a, const Pose& x, const Pose& y) {
  const double ax = a.l1 * std::cos(x.q1) + a.l2 * std::cos(x.q1 + x.q2);
  const double ay = a.l1 * std::sin(x.q1) + a.l2 * std::sin(x.q1 + x.q2);
  const double bx = a.l1 * std::cos(y.q1) + a.l2 * std::cos(y.q1 + y.q2);
  const double by = a.l1 * std::sin(y.q1) + a.l2 * std::sin(y.q1 + y.q2);
  return std::hypot(ax - bx, ay - by);
}

// A joint space controller holding a pose against a gravity that is the real one,
// from the energy. Proportional and damping terms on each joint, and optionally
// the holding torque added as a feedforward. Ten seconds at a kilohertz is long
// enough to settle, and the inertia only decides how it gets there: where it ends
// up is statics.
Pose settle(const ArmBody& a, const Pose& target, const Pose& start, double kp, double kd, bool feedforward) {
  double q1 = start.q1;
  double q2 = start.q2;
  double v1 = 0.0;
  double v2 = 0.0;
  const JointTorques ff = holding_torques(a, target.q1, target.q2, kPayload);
  const double dt = 0.001;

  for (int step = 0; step < 10000; ++step) {
    const JointTorques g = slope_of_energy(a, q1, q2, kPayload);
    const double u1 = kp * (target.q1 - q1) - kd * v1 + (feedforward ? ff.shoulder : 0.0);
    const double u2 = kp * (target.q2 - q2) - kd * v2 + (feedforward ? ff.elbow : 0.0);
    v1 += (u1 - g.shoulder) / kInertia * dt;
    v2 += (u2 - g.elbow) / kInertia * dt;
    q1 += v1 * dt;
    q2 += v2 * dt;
    if (std::fabs(q1) > 50.0) break;   // it has fallen over and is still going
  }
  return Pose{q1, q2};
}

}  // namespace

RC_TEST("the holding torque is the slope of the energy, at every pose and payload tried") {
  const ArmBody arm;
  double worst = 0.0;
  int tried = 0;

  for (int i = -6; i <= 6; ++i) {
    for (int j = -6; j <= 6; ++j) {
      for (const double payload : {0.0, 0.5, 2.0}) {
        const double q1 = static_cast<double>(i) * 0.5;
        const double q2 = static_cast<double>(j) * 0.5;
        const JointTorques mine = holding_torques(arm, q1, q2, payload);
        const JointTorques truth = slope_of_energy(arm, q1, q2, payload);
        worst = std::fmax(worst, std::fmax(std::fabs(mine.shoulder - truth.shoulder),
                                           std::fabs(mine.elbow - truth.elbow)));
        ++tried;
      }
    }
  }

  std::cout << "  " << tried << " poses and payloads, against the slope of the potential energy: the\n"
            << "  worst disagreement was " << std::scientific << std::setprecision(1) << worst << " N m.\n\n";

  RC_CHECK_EQ(tried, 507);
  RC_CHECK(worst < 1e-6);
}

RC_TEST("lying out is the hardest pose, and standing up and hanging down need nothing") {
  const ArmBody arm;
  const JointTorques out = holding_torques(arm, 0.0, 0.0, kPayload);
  const JointTorques up = holding_torques(arm, kHalfPi, 0.0, kPayload);
  const JointTorques down = holding_torques(arm, -kHalfPi, 0.0, kPayload);

  std::cout << "  a 0.5 kg payload on a 0.5 m and 0.4 m arm:\n"
            << "    straight out   shoulder " << std::fixed << std::setprecision(2) << out.shoulder
            << " N m, elbow " << out.elbow << " N m\n"
            << "    straight up    shoulder " << std::fabs(up.shoulder) << " N m, elbow " << std::fabs(up.elbow)
            << " N m\n\n";

  // The weight of everything beyond a joint, times how far out it is.
  RC_CHECK_NEAR(out.elbow, (1.2 * 0.20 + kPayload * 0.4) * 9.81, 1e-9);
  RC_CHECK_NEAR(out.shoulder, (2.0 * 0.25 + (1.2 + kPayload) * 0.5) * 9.81 + out.elbow, 1e-9);
  RC_CHECK(std::fabs(up.shoulder) < 1e-9);
  RC_CHECK(std::fabs(up.elbow) < 1e-9);
  RC_CHECK(std::fabs(down.shoulder) < 1e-9);
  RC_CHECK(std::fabs(down.elbow) < 1e-9);
}

RC_TEST("the torque changes sign once the arm leans back past the vertical") {
  const ArmBody arm;
  const JointTorques forward = holding_torques(arm, 0.3, 0.0, kPayload);
  const JointTorques back = holding_torques(arm, kPi - 0.3, 0.0, kPayload);

  RC_CHECK(forward.shoulder > 0.0);
  RC_CHECK(back.shoulder < 0.0);
  RC_CHECK_NEAR(back.shoulder, -forward.shoulder, 1e-9);   // the mirror image across the vertical
}

RC_TEST("the elbow feels only the angle of the second link, and a payload adds in a straight line") {
  const ArmBody arm;

  // The elbow torque is the second link's weight times the cosine of the angle
  // the second link makes with the horizontal, which is q1 + q2 and not either.
  const JointTorques a = holding_torques(arm, 0.3, 0.5, kPayload);
  const JointTorques b = holding_torques(arm, 0.8, 0.0, kPayload);
  RC_CHECK_NEAR(a.elbow, b.elbow, 1e-12);
  RC_CHECK(std::fabs(a.shoulder - b.shoulder) > 0.5);

  const JointTorques p0 = holding_torques(arm, 0.4, 0.6, 0.0);
  const JointTorques p1 = holding_torques(arm, 0.4, 0.6, 1.0);
  const JointTorques p2 = holding_torques(arm, 0.4, 0.6, 2.0);
  RC_CHECK_NEAR(p2.shoulder - p1.shoulder, p1.shoulder - p0.shoulder, 1e-9);
  RC_CHECK_NEAR(p2.elbow - p1.elbow, p1.elbow - p0.elbow, 1e-9);
}

RC_TEST("the heaviest payload is solved for, and sits exactly on a joint's limit") {
  const ArmBody arm;
  const double shoulder_limit = 20.0;
  const double elbow_limit = 8.0;

  struct Case {
    const char* name;
    double q1;
    double q2;
  };
  const Case cases[] = {{"straight out", 0.0, 0.0},
                        {"raised 45 degrees", kPi / 4.0, 0.0},
                        {"elbow bent", kPi / 4.0, kHalfPi},
                        {"straight up", kHalfPi, 0.0}};

  std::cout << "  the heaviest payload motors of " << shoulder_limit << " and " << elbow_limit
            << " N m can hold:\n";
  for (const Case& c : cases) {
    const double heaviest = heaviest_payload(arm, c.q1, c.q2, shoulder_limit, elbow_limit);
    std::cout << "    " << std::left << std::setw(20) << c.name << std::right;
    if (std::isinf(heaviest)) std::cout << "no limit\n";
    else std::cout << std::fixed << std::setprecision(3) << std::setw(8) << heaviest << " kg\n";

    if (std::isinf(heaviest)) continue;
    const JointTorques at = holding_torques(arm, c.q1, c.q2, heaviest);
    const bool shoulder_on_limit = std::fabs(std::fabs(at.shoulder) - shoulder_limit) < 1e-9;
    const bool elbow_on_limit = std::fabs(std::fabs(at.elbow) - elbow_limit) < 1e-9;
    RC_CHECK(shoulder_on_limit || elbow_on_limit);                // one of them is exactly at its limit
    RC_CHECK(std::fabs(at.shoulder) <= shoulder_limit + 1e-9);    // and neither is over
    RC_CHECK(std::fabs(at.elbow) <= elbow_limit + 1e-9);
  }
  std::cout << "\n";

  // Straight out is the worst, because the whole payload is at the end of the
  // longest lever, and up is no limit at all.
  RC_CHECK(heaviest_payload(arm, 0.0, 0.0, shoulder_limit, elbow_limit) <
           heaviest_payload(arm, kPi / 4.0, 0.0, shoulder_limit, elbow_limit));
  RC_CHECK(std::isinf(heaviest_payload(arm, kHalfPi, 0.0, shoulder_limit, elbow_limit)));

  // An arm that cannot hold itself can hold nothing.
  RC_CHECK_NEAR(heaviest_payload(arm, 0.0, 0.0, 5.0, 2.0), 0.0, 1e-12);
}

RC_TEST("a proportional controller sags under gravity, and feeding the holding torque forward removes it") {
  const ArmBody arm;
  const double kp = 50.0;
  const double kd = 10.0;

  struct Case {
    const char* name;
    Pose pose;
  };
  const Case cases[] = {{"straight out", {0.0, 0.0}},
                        {"raised 45 degrees", {kPi / 4.0, 0.0}},
                        {"elbow bent", {kPi / 4.0, kHalfPi}},
                        {"straight up", {kHalfPi, 0.0}}};

  std::cout << "  holding a pose with a proportional gain of " << std::fixed << std::setprecision(0) << kp
            << " N m per radian and a 0.5 kg payload,\n"
            << "  how far the tool ends up from where it was told to be:\n\n"
            << "    pose                 shoulder torque   proportional only   with the torque fed forward\n";

  double sag_out = 0.0;
  double sag_up = 0.0;
  double worst_with_feedforward = 0.0;
  for (const Case& c : cases) {
    const JointTorques needed = holding_torques(arm, c.pose.q1, c.pose.q2, kPayload);
    const Pose plain = settle(arm, c.pose, c.pose, kp, kd, false);
    const Pose fed = settle(arm, c.pose, c.pose, kp, kd, true);
    const double plain_mm = tool_distance(arm, plain, c.pose) * 1000.0;
    const double fed_mm = tool_distance(arm, fed, c.pose) * 1000.0;

    std::cout << "    " << std::left << std::setw(20) << c.name << std::right << std::fixed << std::setprecision(2)
              << std::setw(12) << needed.shoulder << " N m" << std::setprecision(1) << std::setw(14) << plain_mm
              << " mm" << std::setprecision(4) << std::setw(20) << fed_mm << " mm\n";

    if (std::string(c.name) == "straight out") sag_out = plain_mm;
    if (std::string(c.name) == "straight up") sag_up = plain_mm;
    worst_with_feedforward = std::fmax(worst_with_feedforward, fed_mm);
  }
  std::cout << "\n";

  // The sag is the torque over the gain, and it is a third of a metre: the tool
  // is nowhere near where it was told to go, and the controller is working.
  RC_CHECK(sag_out > 250.0);
  RC_CHECK(sag_up < 1.0);
  RC_CHECK(worst_with_feedforward < 1e-3);
}

RC_TEST("an arm held up from above needs a gain bigger than the slope of gravity, or it falls over") {
  const ArmBody arm;
  const Pose upright{kHalfPi + 0.02, 0.0};   // standing, and two hundredths of a radian off

  // How hard gravity pushes the shoulder further over for each radian it leans, from
  // the holding torque itself: the torque the motor must supply changes by this much
  // per radian, and falls as the arm goes over the top.
  const double h = 1e-4;
  const double slope = -(holding_torques(arm, upright.q1 + h, upright.q2, kPayload).shoulder -
                         holding_torques(arm, upright.q1 - h, upright.q2, kPayload).shoulder) / (2.0 * h);
  std::cout << "  at the top gravity pushes the shoulder over by " << std::fixed << std::setprecision(1) << slope
            << " N m for every radian it leans.\n\n";

  std::cout << "  the arm standing up, nudged 0.02 rad, a proportional gain on each joint and no feedforward:\n";
  double fell_at_10 = 0.0;
  double held_at_50 = 0.0;
  for (const double kp : {10.0, 20.0, 50.0}) {
    const Pose end = settle(arm, upright, upright, kp, 10.0, false);
    std::cout << "    gain " << std::fixed << std::setprecision(0) << std::setw(3) << kp
              << "  ends at a shoulder angle of " << std::setprecision(2) << end.q1 << " rad\n";
    if (kp == 10.0) fell_at_10 = std::fabs(end.q1 - upright.q1);
    if (kp == 50.0) held_at_50 = std::fabs(end.q1 - upright.q1);
  }
  std::cout << "\n  Upright is a hilltop. Gravity pushes the arm further the further it leans, and a gain\n"
            << "  smaller than that push cannot bring it back.\n\n";

  // Ten is below the slope and twenty and fifty are above it.
  RC_CHECK(slope > 10.0 && slope < 20.0);
  RC_CHECK(fell_at_10 > 1.0);
  RC_CHECK(held_at_50 < 0.05);
}
