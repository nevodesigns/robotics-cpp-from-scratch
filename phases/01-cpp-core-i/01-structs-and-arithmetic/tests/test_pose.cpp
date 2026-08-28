#include <rc/test/rc_test.hpp>

#include "solution.hpp"

namespace {
constexpr double kPi = 3.14159265358979323846;
}

RC_TEST("distance between a point and itself is zero") {
  const Pose p{3.0, 4.0, 1.2};
  RC_CHECK_NEAR(distance(p, p), 0.0, 1e-9);
}

RC_TEST("a three four five triangle measures five") {
  RC_CHECK_NEAR(distance(Pose{0.0, 0.0, 0.0}, Pose{3.0, 4.0, 0.0}), 5.0, 1e-9);
}

RC_TEST("distance ignores heading entirely") {
  const Pose facing_east{1.0, 1.0, 0.0};
  const Pose facing_west{1.0, 1.0, kPi};
  RC_CHECK_NEAR(distance(facing_east, facing_west), 0.0, 1e-9);
}

RC_TEST("the midpoint sits halfway along both axes") {
  const Pose middle = midpoint(Pose{0.0, 0.0, 0.0}, Pose{4.0, 10.0, 0.0});
  RC_CHECK_NEAR(middle.x, 2.0, 1e-9);
  RC_CHECK_NEAR(middle.y, 5.0, 1e-9);
  RC_CHECK_NEAR(middle.theta, 0.0, 1e-9);
}

RC_TEST("translating along zero heading moves along x only") {
  const Pose moved = translate(Pose{1.0, 1.0, 0.0}, 2.0);
  RC_CHECK_NEAR(moved.x, 3.0, 1e-9);
  RC_CHECK_NEAR(moved.y, 1.0, 1e-9);
}

RC_TEST("translating along a quarter turn moves along y only") {
  const Pose moved = translate(Pose{0.0, 0.0, kPi / 2.0}, 2.0);
  RC_CHECK_NEAR(moved.x, 0.0, 1e-9);
  RC_CHECK_NEAR(moved.y, 2.0, 1e-9);
}

RC_TEST("translating does not change the heading") {
  const Pose moved = translate(Pose{0.0, 0.0, 0.7}, 5.0);
  RC_CHECK_NEAR(moved.theta, 0.7, 1e-9);
}

RC_TEST("driving forward then back returns to the start") {
  const Pose start{2.0, -1.0, 0.9};
  const Pose there_and_back = translate(translate(start, 3.0), -3.0);
  RC_CHECK_NEAR(distance(start, there_and_back), 0.0, 1e-9);
}
