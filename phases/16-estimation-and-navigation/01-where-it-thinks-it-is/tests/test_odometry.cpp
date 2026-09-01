#include <rc/test/rc_test.hpp>

#include <rc/math/angles.hpp>
#include <rc/sim/diff_drive.hpp>

#include <cmath>
#include <iomanip>
#include <iostream>
#include <random>
#include <string>

#include "solution.hpp"

namespace {

using rc::sim::Pose;

constexpr double kPi = 3.14159265358979323846;
constexpr double kBase = 0.30;
constexpr double kDistance = 100.0;
// One centimetre of commanded travel per update. Fine enough that the numbers
// below are about the errors being studied rather than about the integration,
// and coarse enough that the whole suite runs in well under a second on the
// slowest toolchain this curriculum claims.
constexpr double kStep = 0.01;

Pose at(double x, double y, double theta) {
  Pose p;
  p.x = x; p.y = y; p.theta = theta;
  return p;
}

struct Drift {
  double position = 0.0;
  double heading = 0.0;
};

// One run of a robot that believes some things which are not so.
//
// Truth is integrated with the real wheel base and the distance the wheels
// actually moved along the ground. The estimate is integrated with the believed
// wheel base and the rim travel the encoders reported, which is what a robot
// actually has.
Drift run(bool turning, double true_base, double believed_base, double radius_error,
          double initial_heading_error, double slip, unsigned seed) {
  Odometry truth(true_base, at(0.0, 0.0, 0.0));
  Odometry estimate(believed_base, at(0.0, 0.0, initial_heading_error));

  std::mt19937 rng(seed);

  // Constructed only when there is slip to draw. std::normal_distribution
  // requires a standard deviation greater than zero, and passing zero is a
  // precondition violation rather than a request for no noise: libstdc++
  // happens to return the mean, and the MSVC implementation does not return at
  // all, which presents as a test that times out on one platform and passes on
  // the other two.
  const bool noisy = slip > 0.0;
  std::normal_distribution<double> wander(0.0, noisy ? slip : 1.0);

  const int steps = static_cast<int>(kDistance / kStep);
  const int quarter = steps / 4;
  const int turn_for = static_cast<int>(0.5 / kStep);

  for (int i = 0; i < steps; ++i) {
    double left = kStep;
    double right = kStep;
    if (turning && (i % quarter) >= quarter - turn_for) {
      left = kStep * 0.5;
      right = kStep * 1.5;
    }

    // A wheel that slips turns without the ground moving under it, so the
    // encoder is not wrong: it is answering a different question.
    const double left_ground = noisy ? left * (1.0 - std::fabs(wander(rng))) : left;
    const double right_ground = noisy ? right * (1.0 - std::fabs(wander(rng))) : right;

    truth.update(left_ground, right_ground);
    estimate.update(left * radius_error, right * radius_error);
  }

  return Drift{std::hypot(truth.pose().x - estimate.pose().x,
                          truth.pose().y - estimate.pose().y),
               std::fabs(rc::math::wrap_angle(truth.pose().theta - estimate.pose().theta))};
}

Drift average(bool turning, double tb, double bb, double re, double h0, double slip) {
  if (slip <= 0.0) return run(turning, tb, bb, re, h0, 0.0, 1);
  Drift total;
  const int trials = 8;
  for (int k = 0; k < trials; ++k) {
    const Drift one = run(turning, tb, bb, re, h0, slip, 7 + static_cast<unsigned>(k));
    total.position += one.position;
    total.heading += one.heading;
  }
  total.position /= trials;
  total.heading /= trials;
  return total;
}

}  // namespace

RC_TEST("driving straight moves the estimate along its heading") {
  Odometry odometry(kBase, at(0.0, 0.0, kPi / 2.0));
  for (int i = 0; i < 100; ++i) odometry.update(0.01, 0.01);

  RC_CHECK_NEAR(odometry.pose().x, 0.0, 1e-9);
  RC_CHECK_NEAR(odometry.pose().y, 1.0, 1e-9);
  RC_CHECK_NEAR(odometry.pose().theta, kPi / 2.0, 1e-12);
  RC_CHECK_NEAR(odometry.travelled(), 1.0, 1e-9);
}

RC_TEST("turning in place changes the heading and not the position") {
  Odometry odometry(kBase, at(2.0, -1.0, 0.0));
  for (int i = 0; i < 200; ++i) odometry.update(-0.001, 0.001);

  RC_CHECK_NEAR(odometry.pose().x, 2.0, 1e-9);
  RC_CHECK_NEAR(odometry.pose().y, -1.0, 1e-9);

  // Which way it turned, not merely that it turned. The right wheel going
  // further than the left is a turn to the left, and a sign error here is
  // invisible to any test that compares the estimate against a truth computed
  // with the same code.
  RC_CHECK(odometry.pose().theta > 0.1);
}

RC_TEST("the right wheel going further turns the robot to the left") {
  Odometry left_turn(kBase, at(0.0, 0.0, 0.0));
  left_turn.update(0.0, 0.1);
  RC_CHECK(left_turn.pose().theta > 0.0);

  Odometry right_turn(kBase, at(0.0, 0.0, 0.0));
  right_turn.update(0.1, 0.0);
  RC_CHECK(right_turn.pose().theta < 0.0);
}

RC_TEST("integrating a curve in chunks loses about the length of a chunk") {
  // Every step is a straight line where the robot drove an arc, so the estimate
  // cuts the corner. How much depends on how often it is updated, and the
  // relationship is worth having as a number rather than as an instinct.
  const auto quarter_circle = [](int steps) {
    Odometry odometry(kBase, at(0.0, 0.0, 0.0));
    const double arc = (kPi / 2.0) / steps;          // radius of one metre
    const double turn = (kPi / 2.0) / steps;
    for (int i = 0; i < steps; ++i)
      odometry.update(arc - turn * kBase / 2.0, arc + turn * kBase / 2.0);
    return odometry.pose();
  };

  // Fine enough to serve as the truth: its own error is about a hundredth of
  // the smallest error measured below.
  const Pose truth = quarter_circle(100000);
  std::cout << "\n  a quarter circle of radius 1 m\n\n    "
            << std::left << std::setw(14) << "steps" << std::setw(18) << "step length m"
            << "error m\n";

  double previous = 0.0;
  for (const int steps : {8, 32, 128, 1000}) {
    const Pose estimate = quarter_circle(steps);
    const double error = std::hypot(estimate.x - truth.x, estimate.y - truth.y);
    const double step_length = (kPi / 2.0) / steps;

    std::cout << "    " << std::left << std::setw(14) << steps << std::setw(18)
              << std::fixed << std::setprecision(4) << step_length
              << std::setprecision(6) << error << "\n";

    // Halving the step halves the error, and the error is of the same order as
    // the step itself, which is the whole argument for integrating often.
    RC_CHECK(error < step_length);
    RC_CHECK(error > step_length / 4.0);
    if (previous > 0.0) RC_CHECK(error < previous);
    previous = error;
  }
  std::cout << "\n";
}

RC_TEST("the heading is wrapped, however long the robot runs") {
  // The check that catches an angle growing without limit. Ten full turns is a
  // few seconds of a robot spinning, and an unwrapped heading of sixty three
  // radians compares wrongly against everything.
  Odometry odometry(kBase, at(0.0, 0.0, 0.0));
  const double turn = kBase * 2.0 * kPi * 10.0 / 2.0;   // ten turns of rim travel
  const int steps = 10000;
  for (int i = 0; i < steps; ++i) odometry.update(-turn / steps, turn / steps);

  RC_CHECK(std::fabs(odometry.pose().theta) <= kPi + 1e-9);
}

RC_TEST("the distance travelled counts both directions as distance") {
  Odometry odometry(kBase, at(0.0, 0.0, 0.0));
  odometry.update(0.5, 0.5);
  odometry.update(-0.5, -0.5);

  RC_CHECK_NEAR(odometry.pose().x, 0.0, 1e-12);
  RC_CHECK_NEAR(odometry.travelled(), 1.0, 1e-12);
}

RC_TEST("a correction replaces the estimate and not the odometer") {
  Odometry odometry(kBase, at(0.0, 0.0, 0.0));
  for (int i = 0; i < 100; ++i) odometry.update(0.01, 0.01);
  odometry.correct(at(50.0, 50.0, 1.0));

  RC_CHECK_NEAR(odometry.pose().x, 50.0, 1e-12);

  // How far it has driven since anybody last told it the truth is what says how
  // much to trust it, so the odometer keeps running.
  RC_CHECK_NEAR(odometry.travelled(), 1.0, 1e-9);
}

RC_TEST("perfect wheels on a perfect robot do not drift") {
  const Drift straight = average(false, kBase, kBase, 1.0, 0.0, 0.0);
  const Drift square = average(true, kBase, kBase, 1.0, 0.0, 0.0);

  RC_CHECK(straight.position < 1e-9);
  RC_CHECK(square.position < 1e-9);
}

RC_TEST("which error costs what, and it depends on the path") {
  // The measurement this lesson exists for. Nothing here is a claim about
  // odometry in general; it is what these errors cost this robot over a hundred
  // metres, and the interesting part is that the ranking changes with the path.
  struct Case {
    const char* name;
    double true_base, believed_base, radius, heading, slip;
  };
  const Case cases[] = {
      {"wheel radius 1 percent out",   kBase, kBase,  1.01, 0.0,           0.0},
      {"wheel base 1 percent out",     kBase, kBase * 1.01, 1.0, 0.0,      0.0},
      {"initial heading 1 degree out", kBase, kBase,  1.0,  kPi / 180.0,   0.0},
      {"wheel slip, 1 percent typical",kBase, kBase,  1.0,  0.0,           0.01},
  };

  std::cout << "\n  after " << static_cast<int>(kDistance) << " metres\n\n    "
            << std::left << std::setw(32) << "error source" << std::right
            << std::setw(14) << "straight" << std::setw(14) << "with turns" << "\n";

  double base_straight = -1.0;
  double base_turning = -1.0;
  double heading_straight = -1.0;
  double heading_turning = -1.0;
  double radius_straight = -1.0;
  double slip_straight = -1.0;

  for (const Case& c : cases) {
    const Drift straight =
        average(false, c.true_base, c.believed_base, c.radius, c.heading, c.slip);
    const Drift turning =
        average(true, c.true_base, c.believed_base, c.radius, c.heading, c.slip);

    std::cout << "    " << std::left << std::setw(32) << c.name << std::right << std::fixed
              << std::setprecision(4) << std::setw(12) << straight.position << " m"
              << std::setw(12) << turning.position << " m\n";

    if (std::string(c.name).find("base") != std::string::npos) {
      base_straight = straight.position;
      base_turning = turning.position;
    }
    if (std::string(c.name).find("heading") != std::string::npos) {
      heading_straight = straight.position;
      heading_turning = turning.position;
    }
    if (std::string(c.name).find("radius") != std::string::npos)
      radius_straight = straight.position;
    if (std::string(c.name).find("slip") != std::string::npos)
      slip_straight = straight.position;
  }
  std::cout << "\n";

  // A wheel base that is wrong cannot show while driving straight, because the
  // difference between the wheels is zero and the base only divides that.
  RC_REQUIRE(base_straight >= 0.0);
  RC_CHECK(base_straight < 1e-9);
  RC_CHECK(base_turning > 0.5);

  // And a heading that starts wrong is worst on a long straight run and largely
  // cancels around a closed loop.
  RC_REQUIRE(heading_straight >= 0.0);
  RC_CHECK(heading_straight > 1.0);
  RC_CHECK(heading_turning < heading_straight / 5.0);

  // These four are quoted in the lesson and in E-NAV-0002. Pinning them loosely
  // means a change to the model that invalidates the prose fails here rather
  // than leaving two documents disagreeing with the code.
  RC_CHECK_NEAR(radius_straight, 1.00, 0.05);
  RC_CHECK_NEAR(base_turning, 1.06, 0.10);
  RC_CHECK_NEAR(heading_straight, 1.75, 0.05);
  RC_CHECK_NEAR(slip_straight, 2.22, 0.30);
}

RC_TEST("a heading error costs distance times its tangent, which is a number you can use") {
  // One degree over a hundred metres, worked out on paper and then measured.
  const Drift measured = average(false, kBase, kBase, 1.0, kPi / 180.0, 0.0);
  const double predicted = kDistance * std::tan(kPi / 180.0);

  std::cout << "  one degree of heading error over " << static_cast<int>(kDistance)
            << " metres: predicted " << std::fixed << std::setprecision(4) << predicted
            << " m, measured " << measured.position << " m\n";

  RC_CHECK_NEAR(measured.position, predicted, 0.01);
}

RC_TEST("a wheel radius error costs a fixed fraction of the distance") {
  const Drift measured = average(false, kBase, kBase, 1.01, 0.0, 0.0);
  RC_CHECK_NEAR(measured.position, kDistance * 0.01, 1e-6);
}
