#include <rc/test/rc_test.hpp>

#include "solution.hpp"

RC_TEST("wrapping leaves an in range angle alone") {
  RC_CHECK_NEAR(wrap_angle(0.0), 0.0, 1e-9);
  RC_CHECK_NEAR(wrap_angle(1.5), 1.5, 1e-9);
  RC_CHECK_NEAR(wrap_angle(-1.5), -1.5, 1e-9);
}

RC_TEST("wrapping brings a full turn back to zero") {
  RC_CHECK_NEAR(wrap_angle(2.0 * kPi), 0.0, 1e-9);
  RC_CHECK_NEAR(wrap_angle(-2.0 * kPi), 0.0, 1e-9);
  RC_CHECK_NEAR(wrap_angle(4.0 * kPi), 0.0, 1e-9);
}

RC_TEST("wrapping handles negative angles, which is where fmod trips people") {
  RC_CHECK_NEAR(wrap_angle(-3.0 * kPi / 2.0), kPi / 2.0, 1e-9);
  RC_CHECK_NEAR(wrap_angle(-7.0), -0.71681, 1e-4);
}

RC_TEST("every wrapped angle lands inside the range") {
  for (int i = -50; i <= 50; ++i) {
    const double wrapped = wrap_angle(i * 0.7);
    RC_CHECK(wrapped <= kPi + 1e-9);
    RC_CHECK(wrapped >= -kPi - 1e-9);
  }
}

RC_TEST("the shortest turn across the wrap point is small") {
  // 179 degrees to minus 179 degrees is two degrees, not 358.
  const double from = 179.0 * kPi / 180.0;
  const double to = -179.0 * kPi / 180.0;
  RC_CHECK_NEAR(shortest_turn(from, to), 2.0 * kPi / 180.0, 1e-6);
}

RC_TEST("the shortest turn carries the right sign") {
  RC_CHECK_NEAR(shortest_turn(0.0, 1.0), 1.0, 1e-9);
  RC_CHECK_NEAR(shortest_turn(1.0, 0.0), -1.0, 1e-9);
  RC_CHECK_NEAR(shortest_turn(0.0, 0.0), 0.0, 1e-9);
}

RC_TEST("steering modifies the pose the caller passed in") {
  Pose pose;
  pose.theta = 0.0;
  steer_towards(pose, 1.0, 0.25);
  RC_CHECK_NEAR(pose.theta, 0.25, 1e-9);
}

RC_TEST("steering arrives exactly and then stops") {
  Pose pose;
  pose.theta = 0.0;
  for (int i = 0; i < 100; ++i) steer_towards(pose, 1.0, 0.25);
  RC_CHECK_NEAR(pose.theta, 1.0, 1e-9);
}

RC_TEST("steering across the wrap point takes the short way") {
  Pose pose;
  pose.theta = 3.0;
  steer_towards(pose, -3.0, 0.1);
  // Going the short way means the heading increases past pi and wraps negative.
  RC_CHECK(pose.theta > 3.0 || pose.theta < -3.0);
}
