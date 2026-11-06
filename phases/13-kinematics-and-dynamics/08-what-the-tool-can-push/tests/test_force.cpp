#include <rc/test/rc_test.hpp>

#include <cmath>
#include <iomanip>
#include <iostream>
#include <limits>

#include "solution.hpp"

namespace {

const double kL1 = 0.5;
const double kL2 = 0.4;
const double kShoulderLimit = 20.0;   // newton metres each motor can supply
const double kElbowLimit = 8.0;
const double kRateLimit = 2.0;        // radians a second each joint can turn
const double kPi = 3.141592653589793;

// Where the tool is, written from the geometry and not from the Jacobian.
double tool_x(double q1, double q2) { return kL1 * std::cos(q1) + kL2 * std::cos(q1 + q2); }
double tool_y(double q1, double q2) { return kL1 * std::sin(q1) + kL2 * std::sin(q1 + q2); }

// The torque a force needs, by the principle of virtual work: moving a joint a little
// moves the tool, and the force does work on it, so the torque at the joint is how much
// work the force does per radian. Found by moving each joint a little each way.
JointLoad virtual_work(double q1, double q2, double fx, double fy) {
  const double h = 1e-6;
  const auto work = [&](double a, double b) { return fx * tool_x(a, b) + fy * tool_y(a, b); };
  JointLoad load;
  load.shoulder = (work(q1 + h, q2) - work(q1 - h, q2)) / (2.0 * h);
  load.elbow = (work(q1, q2 + h) - work(q1, q2 - h)) / (2.0 * h);
  return load;
}

// The fastest the tool can move in a direction with the joints held to kRateLimit, from
// the inverse Jacobian: the speed at which the first joint reaches its limit.
double fastest_move(double q1, double q2, double ux, double uy) {
  const PlanarJacobian j = planar_jacobian(kL1, kL2, q1, q2);
  const double det = j.determinant();
  if (std::fabs(det) < 1e-12) return 0.0;

  // The joint rates for a unit speed in that direction: the inverse of J times u.
  const double r1 = std::fabs((j.d * ux - j.b * uy) / det);
  const double r2 = std::fabs((-j.c * ux + j.a * uy) / det);
  const double worst = r1 > r2 ? r1 : r2;
  return worst < 1e-12 ? std::numeric_limits<double>::infinity() : kRateLimit / worst;
}

}  // namespace

RC_TEST("the torques for a force are the transpose of the Jacobian, as virtual work says") {
  double worst = 0.0;
  int tried = 0;
  for (int i = -6; i <= 6; ++i) {
    for (int k = -6; k <= 6; ++k) {
      const double q1 = static_cast<double>(i) * 0.5;
      const double q2 = static_cast<double>(k) * 0.5;
      for (const double fx : {-7.0, 0.0, 12.0}) {
        for (const double fy : {-3.0, 5.0}) {
          const PlanarJacobian j = planar_jacobian(kL1, kL2, q1, q2);
          const JointLoad mine = torques_for_force(j, ToolForce{fx, fy});
          const JointLoad truth = virtual_work(q1, q2, fx, fy);
          worst = std::fmax(worst, std::fmax(std::fabs(mine.shoulder - truth.shoulder),
                                             std::fabs(mine.elbow - truth.elbow)));
          ++tried;
        }
      }
    }
  }

  std::cout << "  " << tried << " poses and forces, against the work the force does as each joint moves:\n"
            << "  the worst disagreement was " << std::scientific << std::setprecision(1) << worst << " N m.\n\n";

  // The matrix the wrong way up, which is the mistake the transpose is there to prevent.
  const PlanarJacobian j = planar_jacobian(kL1, kL2, 0.4, 1.1);
  const JointLoad right = torques_for_force(j, ToolForce{10.0, 0.0});
  const double wrong_shoulder = j.a * 10.0;
  const double wrong_elbow = j.c * 10.0;
  std::cout << "  holding 10 N along x at a pose of (0.4, 1.1):\n"
            << "    with the transpose        shoulder " << std::fixed << std::setprecision(2) << right.shoulder
            << ", elbow " << right.elbow << " N m\n"
            << "    with the matrix as it is  shoulder " << wrong_shoulder << ", elbow " << wrong_elbow << " N m\n\n";

  RC_CHECK_EQ(tried, 1014);
  RC_CHECK(worst < 1e-7);
  RC_CHECK(std::fabs(right.shoulder - wrong_shoulder) > 0.5 || std::fabs(right.elbow - wrong_elbow) > 0.5);
}

RC_TEST("the force a pair of torques amounts to is found, and is not there at full extension") {
  const PlanarJacobian bent = planar_jacobian(kL1, kL2, 0.4, 1.1);
  bool ok = false;
  for (const double fx : {-9.0, 2.5, 30.0}) {
    for (const double fy : {-4.0, 0.0, 11.0}) {
      const JointLoad load = torques_for_force(bent, ToolForce{fx, fy});
      const ToolForce back = force_for_torques(bent, load.shoulder, load.elbow, ok);
      RC_REQUIRE(ok);
      RC_CHECK_NEAR(back.fx, fx, 1e-9);
      RC_CHECK_NEAR(back.fy, fy, 1e-9);
    }
  }

  // Straight out, a force along the arm puts no torque on either joint, so torques
  // cannot tell you about it.
  const PlanarJacobian straight = planar_jacobian(kL1, kL2, 0.4, 0.0);
  const JointLoad axial = torques_for_force(straight, ToolForce{20.0 * std::cos(0.4), 20.0 * std::sin(0.4)});
  RC_CHECK_NEAR(axial.shoulder, 0.0, 1e-9);
  RC_CHECK_NEAR(axial.elbow, 0.0, 1e-9);

  force_for_torques(straight, 1.0, 1.0, ok);
  RC_CHECK(!ok);

  // Nearly straight is solvable, and shows how badly conditioned it is: a tenth of a newton
  // metre of error in the torques becomes a very large error in the force.
  const PlanarJacobian nearly = planar_jacobian(kL1, kL2, 0.4, 0.01);
  const ToolForce small = force_for_torques(nearly, 1.0, 1.0, ok);
  const ToolForce noisy = force_for_torques(nearly, 1.1, 1.0, ok);
  RC_REQUIRE(ok);
  const double amplified = std::hypot(noisy.fx - small.fx, noisy.fy - small.fy);
  std::cout << "  with the elbow at 0.01 rad, a change of 0.1 N m in one torque changes the force it\n"
            << "  implies by " << std::fixed << std::setprecision(1) << amplified << " N.\n\n";
  RC_CHECK(amplified > 5.0);
}

RC_TEST("the hardest push puts exactly one joint on its limit") {
  const PlanarJacobian j = planar_jacobian(kL1, kL2, 0.3, 0.9);
  int tried = 0;
  for (int d = 0; d < 24; ++d) {
    const double angle = static_cast<double>(d) * kPi / 12.0;
    const double ux = std::cos(angle);
    const double uy = std::sin(angle);

    const double push = strongest_push(j, ux, uy, kShoulderLimit, kElbowLimit);
    RC_REQUIRE(std::isfinite(push));
    const JointLoad load = torques_for_force(j, ToolForce{ux * push, uy * push});

    const bool shoulder_on_limit = std::fabs(std::fabs(load.shoulder) - kShoulderLimit) < 1e-9;
    const bool elbow_on_limit = std::fabs(std::fabs(load.elbow) - kElbowLimit) < 1e-9;
    RC_CHECK(shoulder_on_limit || elbow_on_limit);
    RC_CHECK(std::fabs(load.shoulder) <= kShoulderLimit + 1e-9);
    RC_CHECK(std::fabs(load.elbow) <= kElbowLimit + 1e-9);
    ++tried;
  }
  RC_CHECK_EQ(tried, 24);

  // The size of the direction does not matter, and no direction is no push.
  RC_CHECK_NEAR(strongest_push(j, 5.0, 0.0, kShoulderLimit, kElbowLimit),
                strongest_push(j, 1.0, 0.0, kShoulderLimit, kElbowLimit), 1e-9);
  RC_CHECK_NEAR(strongest_push(j, 0.0, 0.0, kShoulderLimit, kElbowLimit), 0.0, 1e-12);
}

RC_TEST("what the arm can push depends on the direction, and on the elbow") {
  const double q1 = 0.4;
  const double elbows[] = {0.05, 0.5, 1.0, kPi / 2.0, 2.5};

  std::cout << "  the hardest the tool can push, with motors of 20 and 8 N m, along the arm and across it,\n"
            << "  and the fastest it can move in the same two directions with each joint held to 2 rad/s:\n\n"
            << "    elbow      push along    push across    move along    move across\n"
            << "    (rad)           (N)            (N)         (m/s)          (m/s)\n";

  double along_first = 0.0;
  double across_first = 0.0;
  double speed_along_first = 0.0;
  double speed_across_first = 0.0;
  double across_square = 0.0;
  for (int i = 0; i < 5; ++i) {
    const double q2 = elbows[i];
    const PlanarJacobian j = planar_jacobian(kL1, kL2, q1, q2);

    // The direction from the shoulder to the tool, which is the way the arm points.
    const double px = tool_x(q1, q2);
    const double py = tool_y(q1, q2);
    const double reach = std::hypot(px, py);
    const double ax = px / reach;
    const double ay = py / reach;

    const double along = strongest_push(j, ax, ay, kShoulderLimit, kElbowLimit);
    const double across = strongest_push(j, -ay, ax, kShoulderLimit, kElbowLimit);
    const double move_along = fastest_move(q1, q2, ax, ay);
    const double move_across = fastest_move(q1, q2, -ay, ax);

    std::cout << "    " << std::fixed << std::setprecision(2) << std::setw(5) << q2 << std::setprecision(1)
              << std::setw(14) << along << std::setw(15) << across << std::setprecision(2) << std::setw(14)
              << move_along << std::setw(15) << move_across << "\n";

    if (i == 0) {
      along_first = along;
      across_first = across;
      speed_along_first = move_along;
      speed_across_first = move_across;
    }
    if (i == 3) across_square = across;
  }
  std::cout << "\n  Nearly straight, the arm can push along itself " << std::setprecision(0)
            << along_first / across_first << " times harder than across itself and moves along itself\n"
            << "  " << std::setprecision(0) << speed_across_first / speed_along_first
            << " times slower: the direction it cannot move in is the direction it can push hardest.\n\n";

  // Nearly straight: along the arm the structure carries the load and the motors hardly feel
  // it, across the arm they carry all of it.
  RC_CHECK(along_first > 10.0 * across_first);
  RC_CHECK(speed_along_first < 0.1 * speed_across_first);
  RC_CHECK(across_first < across_square * 2.0);
}

RC_TEST("a straight arm carries a load along itself for free, and the tool can push without limit") {
  const PlanarJacobian j = planar_jacobian(kL1, kL2, 0.4, 0.0);
  const double along = strongest_push(j, std::cos(0.4), std::sin(0.4), kShoulderLimit, kElbowLimit);
  RC_CHECK(std::isinf(along));

  // And across it the same arm is limited by its motors, at the reach times the force.
  const double across = strongest_push(j, -std::sin(0.4), std::cos(0.4), kShoulderLimit, kElbowLimit);
  RC_CHECK(std::isfinite(across));
  const double by_shoulder = kShoulderLimit / (kL1 + kL2);
  const double by_elbow = kElbowLimit / kL2;
  RC_CHECK_NEAR(across, by_shoulder < by_elbow ? by_shoulder : by_elbow, 1e-9);
}
