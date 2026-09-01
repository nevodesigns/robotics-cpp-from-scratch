#include <rc/test/rc_test.hpp>

#include <rc/sim/diff_drive.hpp>

#include <cmath>
#include <vector>

#include "solution.hpp"

namespace {

using rc::sim::Pose;

constexpr double kPi = 3.14159265358979323846;
constexpr double kTolerance = 1e-9;
constexpr double kWheelBase = 0.3;
constexpr double kDt = 0.01;

Pose at(double x, double y, double theta) {
  Pose pose;
  pose.x = x;
  pose.y = y;
  pose.theta = theta;
  return pose;
}

}  // namespace

RC_TEST("distance is measured the way a ruler measures it") {
  RC_CHECK_NEAR(distance_between(at(0, 0, 0), at(3, 4, 0)), 5.0, kTolerance);
  RC_CHECK_NEAR(distance_between(at(0, 0, 0), at(0, 0, 0)), 0.0, kTolerance);

  // Heading is not distance. Two robots in the same place facing opposite ways
  // are nought metres apart.
  RC_CHECK_NEAR(distance_between(at(1, 1, 0), at(1, 1, kPi)), 0.0, kTolerance);
}

RC_TEST("the difference between two headings is the short way round") {
  // The check that catches a subtraction. From 179 degrees to minus 179 is a
  // turn of two degrees, and subtracting says 358.
  const double from = 179.0 * kPi / 180.0;
  const double to = -179.0 * kPi / 180.0;

  RC_CHECK_NEAR(std::fabs(heading_difference(from, to)), 2.0 * kPi / 180.0, 1e-9);
  RC_CHECK(std::fabs(heading_difference(from, to)) < 0.1);
}

RC_TEST("the difference between two headings keeps its sign") {
  RC_CHECK_NEAR(heading_difference(0.0, 0.5), 0.5, kTolerance);
  RC_CHECK_NEAR(heading_difference(0.5, 0.0), -0.5, kTolerance);
  RC_CHECK_NEAR(heading_difference(1.0, 1.0), 0.0, kTolerance);
}

RC_TEST("two names for the same heading compare equal") {
  // Pi and minus pi are the same direction written two ways, and a comparison
  // that says otherwise reports a bug that is not there.
  RC_CHECK(same_heading(kPi, -kPi, 1e-9));
  RC_CHECK(same_heading(0.0, 2.0 * kPi, 1e-9));
  RC_CHECK(same_heading(0.1, 0.1 + 4.0 * kPi, 1e-9));
  RC_CHECK(!same_heading(0.0, kPi, 1e-9));
}

RC_TEST("comparisons allow for the last few bits") {
  // The check that catches an exact equality. These two numbers should be the
  // same and are not, for reasons that have nothing to do with the robot.
  double accumulated = 0.0;
  for (int i = 0; i < 10; ++i) accumulated += 0.1;

  RC_CHECK(accumulated != 1.0);                       // they really do differ
  RC_CHECK(same_position(at(accumulated, 0, 0), at(1.0, 0, 0), 1e-9));
}

RC_TEST("same pose asks about both the place and the direction") {
  RC_CHECK(same_pose(at(1, 2, 0.5), at(1, 2, 0.5), kTolerance));
  RC_CHECK(!same_pose(at(1, 2, 0.5), at(1, 2, 1.5), kTolerance));   // same place
  RC_CHECK(!same_pose(at(1, 2, 0.5), at(9, 2, 0.5), kTolerance));   // same heading
}

RC_TEST("the distance covered is checked against speed and time") {
  RC_CHECK(moved_expected_distance(at(0, 0, 0), at(2, 0, 0), 1.0, 2.0, 1e-9));
  RC_CHECK(moved_expected_distance(at(0, 0, 0), at(0, 2, 0), 1.0, 2.0, 1e-9));

  // Half the distance it should have covered, which is what a model that
  // averages one time too many produces.
  RC_CHECK(!moved_expected_distance(at(0, 0, 0), at(1, 0, 0), 1.0, 2.0, 1e-9));

  // Driving backwards still covers distance.
  RC_CHECK(moved_expected_distance(at(0, 0, 0), at(-2, 0, 0), -1.0, 2.0, 1e-9));
}

// ---------------------------------------------------------------------------
// The checks applied to a real trajectory. These are the questions worth asking
// of any model that moves something.
// ---------------------------------------------------------------------------

RC_TEST("driving straight does not change the heading") {
  const Pose start = at(0.0, 0.0, 0.7);
  Pose pose = start;
  for (int i = 0; i < 500; ++i) pose = rc::sim::step(pose, 1.0, 1.0, kWheelBase, kDt);

  RC_CHECK(same_heading(pose.theta, start.theta, 1e-9));
  RC_CHECK(!same_position(pose, start, 1e-9));   // and it did actually move
}

RC_TEST("spinning in place does not change the position") {
  const Pose start = at(2.0, -1.0, 0.0);
  Pose pose = start;
  for (int i = 0; i < 500; ++i) pose = rc::sim::step(pose, -0.5, 0.5, kWheelBase, kDt);

  RC_CHECK(same_position(pose, start, 1e-9));
  RC_CHECK(!same_heading(pose.theta, start.theta, 1e-9));   // and it did turn
}

RC_TEST("driving there and back returns the robot to where it started") {
  const Pose start = at(1.0, 2.0, 0.3);
  Pose pose = start;
  for (int i = 0; i < 200; ++i) pose = rc::sim::step(pose, 1.0, 1.0, kWheelBase, kDt);
  for (int i = 0; i < 200; ++i) pose = rc::sim::step(pose, -1.0, -1.0, kWheelBase, kDt);

  // Not exactly, and that is the point of a tolerance: two hundred additions of
  // a fraction do not undo two hundred subtractions of it to the last bit.
  RC_CHECK(same_pose(pose, start, 1e-9));
}

RC_TEST("the distance covered matches the speed and the time") {
  const Pose start = at(0.0, 0.0, 1.1);
  Pose pose = start;
  const int steps = 300;
  for (int i = 0; i < steps; ++i) pose = rc::sim::step(pose, 2.0, 2.0, kWheelBase, kDt);

  RC_CHECK(moved_expected_distance(start, pose, 2.0, steps * kDt, 1e-9));
}

// ---------------------------------------------------------------------------
// A check that cannot fail is not a check. Each of these feeds the checks a
// trajectory from a model that is wrong in one specific way, and requires them
// to notice.
// ---------------------------------------------------------------------------

RC_TEST("the checks notice a heading that drifts") {
  const Pose start = at(0.0, 0.0, 0.0);
  Pose pose = start;
  for (int i = 0; i < 500; ++i) {
    pose = rc::sim::step(pose, 1.0, 1.0, kWheelBase, kDt);
    pose.theta += 1e-4;   // a turn rate that should have been zero
  }
  RC_CHECK(!same_heading(pose.theta, start.theta, 1e-9));
}

RC_TEST("the checks notice a trajectory of the wrong size") {
  const Pose start = at(0.0, 0.0, 0.0);
  Pose pose = start;
  const int steps = 300;
  for (int i = 0; i < steps; ++i) pose = rc::sim::step(pose, 2.0, 2.0, kWheelBase, kDt / 2.0);

  // Every pose is plausible and the whole path is half the length it should be.
  RC_CHECK(!moved_expected_distance(start, pose, 2.0, steps * kDt, 1e-9));
}

RC_TEST("the checks notice a spin that wanders") {
  const Pose start = at(0.0, 0.0, 0.0);
  Pose pose = start;
  for (int i = 0; i < 500; ++i) {
    pose = rc::sim::step(pose, -0.5, 0.5, kWheelBase, kDt);
    pose.x += 1e-5;   // a spin that should not have gone anywhere
  }
  RC_CHECK(!same_position(pose, start, 1e-9));
}
