#include <rc/test/rc_test.hpp>

#include <iostream>
#include <string>
#include <vector>

#include "solution.hpp"

namespace {

constexpr double kPi = 3.14159265358979323846;
constexpr double kWheelBase = 0.30;

// Draws the path onto a small character grid so you can see the robot move.
// This is a test helper, not something you implement.
std::string render(const std::vector<Pose>& path, double metres_per_cell) {
  const int width = 40;
  const int height = 13;
  std::vector<std::string> grid(height, std::string(width, '.'));

  for (const Pose& p : path) {
    const int column = static_cast<int>(p.x / metres_per_cell) + width / 4;
    const int row = height / 2 - static_cast<int>(p.y / metres_per_cell);
    if (column >= 0 && column < width && row >= 0 && row < height) grid[row][column] = 'o';
  }
  if (!path.empty()) {
    const Pose& last = path.back();
    const int column = static_cast<int>(last.x / metres_per_cell) + width / 4;
    const int row = height / 2 - static_cast<int>(last.y / metres_per_cell);
    if (column >= 0 && column < width && row >= 0 && row < height) grid[row][column] = 'R';
  }

  std::string out = "\n";
  for (const std::string& line : grid) out += "    " + line + "\n";
  return out;
}

}  // namespace

RC_TEST("a stopped robot stays exactly where it is") {
  const Pose start{1.0, 2.0, 0.5};
  const Pose next = step(start, 0.0, 0.0, kWheelBase, 0.1);
  RC_CHECK_NEAR(next.x, 1.0, 1e-9);
  RC_CHECK_NEAR(next.y, 2.0, 1e-9);
  RC_CHECK_NEAR(next.theta, 0.5, 1e-9);
}

RC_TEST("equal wheel speeds drive straight along the heading") {
  const Pose next = step(Pose{}, 1.0, 1.0, kWheelBase, 2.0);
  RC_CHECK_NEAR(next.x, 2.0, 1e-9);
  RC_CHECK_NEAR(next.y, 0.0, 1e-9);
  RC_CHECK_NEAR(next.theta, 0.0, 1e-9);
}

RC_TEST("facing along y, the robot moves along y") {
  // This is the check that catches a swapped sine and cosine.
  const Pose start{0.0, 0.0, kPi / 2.0};
  const Pose next = step(start, 1.0, 1.0, kWheelBase, 1.0);
  RC_CHECK_NEAR(next.x, 0.0, 1e-9);
  RC_CHECK_NEAR(next.y, 1.0, 1e-9);
}

RC_TEST("opposite wheel speeds spin in place") {
  const Pose next = step(Pose{}, -0.15, 0.15, kWheelBase, 1.0);
  RC_CHECK_NEAR(next.x, 0.0, 1e-9);
  RC_CHECK_NEAR(next.y, 0.0, 1e-9);
  RC_CHECK_NEAR(next.theta, 1.0, 1e-9);
}

RC_TEST("the heading stays inside minus pi to pi") {
  Pose pose;
  for (int i = 0; i < 200; ++i) pose = step(pose, -0.15, 0.15, kWheelBase, 0.1);
  RC_CHECK(pose.theta <= kPi);
  RC_CHECK(pose.theta >= -kPi);
}

RC_TEST("a faster right wheel curves the robot to the left") {
  Pose pose;
  for (int i = 0; i < 50; ++i) pose = step(pose, 0.8, 1.0, kWheelBase, 0.05);
  RC_CHECK(pose.y > 0.0);
  RC_CHECK(pose.theta > 0.0);
}

RC_TEST("your robot drives a quarter circle") {
  // Turn rate here is (0.75 - 0.55) / 0.30, which is two thirds of a radian per
  // second. A quarter turn is pi/2 radians, so it takes about 2.36 seconds,
  // which is 47 steps of 0.05 seconds.
  std::vector<Pose> path;
  Pose pose;
  for (int i = 0; i < 47; ++i) {
    pose = step(pose, 0.55, 0.75, kWheelBase, 0.05);
    path.push_back(pose);
  }
  std::cout << "\n  the path your robot drove:\n" << render(path, 0.12);

  RC_CHECK_NEAR(path.back().theta, kPi / 2.0, 0.05);
  RC_CHECK(path.back().x > 0.0);
  RC_CHECK(path.back().y > 0.0);
}
