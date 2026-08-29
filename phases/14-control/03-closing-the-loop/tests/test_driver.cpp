#include <rc/test/rc_test.hpp>

#include <cmath>
#include <vector>

#include "solution.hpp"

namespace {

constexpr double kPi = 3.14159265358979323846;
constexpr double kDt = 0.01;

rc::sim::Pose at(double x, double y, double theta = 0.0) {
  rc::sim::Pose pose;
  pose.x = x;
  pose.y = y;
  pose.theta = theta;
  return pose;
}

struct Run {
  rc::sim::Pose final_pose;
  bool arrived = false;
  int steps = 0;
  double fastest_wheel = 0.0;
};

// Drives the simulated robot with the controller under test, which is the whole
// point of this lesson: the model from phase 00, the controller from 14-01 and
// the watchdog from 14-02, working as one system.
Run drive_to(const rc::sim::Pose& start, const rc::sim::Pose& target, int max_steps = 4000) {
  WaypointDriver driver;
  Run run;
  run.final_pose = start;

  for (run.steps = 0; run.steps < max_steps; ++run.steps) {
    const double now = run.steps * kDt;
    driver.set_target(target, now);
    const WheelSpeeds speeds = driver.command(run.final_pose, now, kDt);

    run.fastest_wheel = std::max(run.fastest_wheel,
                                 std::max(std::fabs(speeds.left), std::fabs(speeds.right)));
    run.final_pose = rc::sim::step(run.final_pose, speeds.left, speeds.right, 0.30, kDt);

    if (driver.arrived(run.final_pose)) {
      run.arrived = true;
      break;
    }
  }
  return run;
}

}  // namespace

RC_TEST("a target straight ahead has no heading error") {
  RC_CHECK_NEAR(heading_error(at(0, 0, 0), at(2, 0)), 0.0, 1e-9);
}

RC_TEST("a target to the left is a positive quarter turn") {
  RC_CHECK_NEAR(heading_error(at(0, 0, 0), at(0, 2)), kPi / 2.0, 1e-9);
}

RC_TEST("heading error accounts for where the robot is already facing") {
  RC_CHECK_NEAR(heading_error(at(0, 0, kPi / 2.0), at(0, 2)), 0.0, 1e-9);
}

RC_TEST("heading error takes the short way round") {
  // The robot faces just left of straight back, and the target is just right of
  // straight back. The answer is a small turn, not almost a full circle.
  const double error = heading_error(at(0, 0, 3.0), at(-1.0, -0.05));
  RC_CHECK(std::fabs(error) < 0.5);
}

RC_TEST("equal wheel speeds mean straight ahead") {
  const WheelSpeeds speeds = to_wheel_speeds(0.5, 0.0, 0.30);
  RC_CHECK_NEAR(speeds.left, 0.5, 1e-9);
  RC_CHECK_NEAR(speeds.right, 0.5, 1e-9);
}

RC_TEST("turning in place means equal and opposite wheels") {
  const WheelSpeeds speeds = to_wheel_speeds(0.0, 1.0, 0.30);
  RC_CHECK_NEAR(speeds.left, -0.15, 1e-9);
  RC_CHECK_NEAR(speeds.right, 0.15, 1e-9);
}

RC_TEST("wheel speeds are the exact inverse of the phase 00 model") {
  // Ask for a forward speed and a turn rate, convert to wheels, and run those
  // wheels through the simulator. The robot must move exactly as asked.
  const double forward = 0.4;
  const double turn = 0.9;
  const WheelSpeeds speeds = to_wheel_speeds(forward, turn, 0.30);

  const rc::sim::Pose start = at(0, 0, 0);
  const rc::sim::Pose moved = rc::sim::step(start, speeds.left, speeds.right, 0.30, 0.1);

  RC_CHECK_NEAR(moved.x, forward * 0.1, 1e-9);
  RC_CHECK_NEAR(moved.theta, turn * 0.1, 1e-9);
}

RC_TEST("a driver that was never given a target does not move") {
  // The watchdog has never been fed, which counts as expired. This is the check
  // that catches a watchdog that begins life trusting.
  WaypointDriver driver;
  const WheelSpeeds speeds = driver.command(at(0, 0, 0), 0.0, kDt);
  RC_CHECK_NEAR(speeds.left, 0.0, 1e-12);
  RC_CHECK_NEAR(speeds.right, 0.0, 1e-12);
}

RC_TEST("a target that stops arriving brings the robot to a stop") {
  WaypointDriver driver;
  driver.set_target(at(5, 0), 1.0);

  const WheelSpeeds moving = driver.command(at(0, 0, 0), 1.1, kDt);
  RC_CHECK(std::fabs(moving.left) + std::fabs(moving.right) > 0.1);

  // Half a second later with no fresh target, the robot must stop rather than
  // drive on towards a goal nobody has confirmed.
  const WheelSpeeds stopped = driver.command(at(0, 0, 0), 2.0, kDt);
  RC_CHECK_NEAR(stopped.left, 0.0, 1e-12);
  RC_CHECK_NEAR(stopped.right, 0.0, 1e-12);
}

RC_TEST("the robot reaches a target straight ahead") {
  const Run run = drive_to(at(0, 0, 0), at(2, 0));
  RC_CHECK(run.arrived);
}

RC_TEST("the robot reaches a target to one side") {
  const Run run = drive_to(at(0, 0, 0), at(0, 2));
  RC_CHECK(run.arrived);
}

RC_TEST("the robot turns around and reaches a target behind it") {
  // The hardest case, and the one a controller without the turn first rule
  // fails by arcing away before it comes round.
  const Run run = drive_to(at(0, 0, 0), at(-2, 0));
  RC_CHECK(run.arrived);
}

RC_TEST("the robot reaches targets in every quarter") {
  const std::vector<rc::sim::Pose> targets = {at(1.5, 1.5), at(-1.5, 1.5), at(-1.5, -1.5),
                                              at(1.5, -1.5)};
  for (const rc::sim::Pose& target : targets) {
    const Run run = drive_to(at(0, 0, 0), target);
    RC_CHECK(run.arrived);
  }
}

RC_TEST("no wheel is ever commanded beyond its limit") {
  const Run run = drive_to(at(0, 0, 0), at(-2, 1));
  RC_CHECK(run.arrived);
  RC_CHECK(run.fastest_wheel <= 0.6 + 1e-9);
}

RC_TEST("the robot stops once it has arrived rather than circling") {
  WaypointDriver driver;
  const rc::sim::Pose target = at(1.0, 0.0);
  driver.set_target(target, 0.0);

  const WheelSpeeds speeds = driver.command(at(0.99, 0.0, 0.0), 0.0, kDt);
  RC_CHECK_NEAR(speeds.left, 0.0, 1e-12);
  RC_CHECK_NEAR(speeds.right, 0.0, 1e-12);
}

RC_TEST("the robot settles rather than overshooting and coming back") {
  // Keep driving well past arrival. A controller that does not slow on approach
  // sails past and circles, which shows up as the distance growing again.
  WaypointDriver driver;
  const rc::sim::Pose target = at(1.5, 0.0);
  rc::sim::Pose pose = at(0, 0, 0);

  double worst_after_arrival = 0.0;
  bool has_arrived = false;
  for (int i = 0; i < 3000; ++i) {
    const double now = i * kDt;
    driver.set_target(target, now);
    const WheelSpeeds speeds = driver.command(pose, now, kDt);
    pose = rc::sim::step(pose, speeds.left, speeds.right, 0.30, kDt);

    if (driver.arrived(pose)) has_arrived = true;
    if (has_arrived) worst_after_arrival = std::max(worst_after_arrival, distance_to(pose, target));
  }

  RC_REQUIRE(has_arrived);
  RC_CHECK(worst_after_arrival <= 0.06);
}
