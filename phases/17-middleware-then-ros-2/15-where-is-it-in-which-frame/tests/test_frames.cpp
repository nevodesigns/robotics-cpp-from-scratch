#include <rc/test/rc_test.hpp>

#include <cmath>
#include <iomanip>
#include <iostream>

#include "solution.hpp"

namespace {
const double kPi = 3.14159265358979323846;
}

RC_TEST("a transform followed by its inverse is nothing, and the heading stays within a half turn either way") {
  const Transform2 t{3.0, -2.0, 2.5};
  const Transform2 id = compose(inverse(t), t);
  RC_CHECK_NEAR(id.x, 0.0, 1e-12);
  RC_CHECK_NEAR(id.y, 0.0, 1e-12);
  RC_CHECK_NEAR(id.theta, 0.0, 1e-12);
  const Transform2 turn = compose(Transform2{0, 0, 3.0}, Transform2{0, 0, 3.0});   // 6.0 rad is more than a half turn
  RC_CHECK_NEAR(turn.theta, 6.0 - 2.0 * kPi, 1e-12);
  double ox = 0.0;
  double oy = 0.0;
  apply(Transform2{1.0, 2.0, kPi / 2.0}, 1.0, 0.0, ox, oy);                        // turned a quarter, then moved
  RC_CHECK_NEAR(ox, 1.0, 1e-12);
  RC_CHECK_NEAR(oy, 3.0, 1e-12);
  const Transform2 half = blend(Transform2{0, 0, 0}, Transform2{10, 20, 0}, 0.5);
  RC_CHECK_NEAR(half.x, 5.0, 1e-12);
  RC_CHECK_NEAR(half.y, 10.0, 1e-12);
}

RC_TEST("the short way round: a heading halfway between 170 and minus 170 degrees is 180, not 0") {
  const double d = kPi / 180.0;
  const Transform2 mid = blend(Transform2{0, 0, 170.0 * d}, Transform2{0, 0, -170.0 * d}, 0.5);
  std::cout << "  halfway between 170 and -170 degrees: " << std::fixed << std::setprecision(1) << std::fabs(mid.theta) / d << " degrees\n\n";
  RC_CHECK_NEAR(std::fabs(mid.theta), kPi, 1e-9);
  const Transform2 near_zero = blend(Transform2{0, 0, -10.0 * d}, Transform2{0, 0, 10.0 * d}, 0.5);
  RC_CHECK_NEAR(near_zero.theta, 0.0, 1e-9);
}

RC_TEST("a point seen by the laser is placed in the map by composing the links on the way up the tree and down") {
  FrameTree tree;
  tree.set("odom", "map", 0.0, Transform2{10.0, 5.0, 0.0});                          // odometry's origin is at (10, 5) in the map
  tree.set("base", "odom", 0.0, Transform2{2.0, 0.0, kPi / 2.0});                    // the robot at (2, 0) in odom, facing up
  tree.set_static("laser", "base", Transform2{0.3, 0.0, 0.0});                       // the laser 30 cm in front of the base
  tree.set_static("arm", "base", Transform2{-0.1, 0.2, 0.0});
  Transform2 map_from_laser;
  RC_CHECK(tree.lookup("map", "laser", 0.0, map_from_laser) == LookupStatus::ok);
  double mx = 0.0;
  double my = 0.0;
  apply(map_from_laser, 1.0, 0.0, mx, my);                                           // an obstacle 1 m ahead of the laser
  std::cout << "  an obstacle 1 m ahead of the laser, robot at (2,0) in odom facing up, odom at (10,5) in the map: map position (" << std::setprecision(2) << mx
            << ", " << my << ")\n\n";
  RC_CHECK_NEAR(mx, 12.0, 1e-9);
  RC_CHECK_NEAR(my, 6.3, 1e-9);
  Transform2 laser_from_arm;
  RC_CHECK(tree.lookup("laser", "arm", 0.0, laser_from_arm) == LookupStatus::ok);    // two frames on different branches of the same parent
  RC_CHECK_NEAR(laser_from_arm.x, -0.4, 1e-9);
  RC_CHECK_NEAR(laser_from_arm.y, 0.2, 1e-9);
  Transform2 same;
  RC_CHECK(tree.lookup("base", "base", 0.0, same) == LookupStatus::ok);
  RC_CHECK_NEAR(same.x, 0.0, 1e-12);
  Transform2 back;
  RC_CHECK(tree.lookup("laser", "map", 0.0, back) == LookupStatus::ok);              // and the other way
  double lx = 0.0;
  double ly = 0.0;
  apply(back, 12.0, 6.3, lx, ly);
  RC_CHECK_NEAR(lx, 1.0, 1e-9);
  RC_CHECK_NEAR(ly, 0.0, 1e-9);
}

RC_TEST("between two reports a frame is where it would have been, so a robot driving at a steady speed is found at the midpoint") {
  FrameTree tree;
  tree.set("base", "odom", 0.0, Transform2{0.0, 0.0, 0.0});
  tree.set("base", "odom", 1.0, Transform2{0.6, 0.0, 0.0});
  Transform2 t;
  RC_CHECK(tree.lookup("odom", "base", 0.5, t) == LookupStatus::ok);
  RC_CHECK_NEAR(t.x, 0.3, 1e-12);
  RC_CHECK(tree.lookup("odom", "base", 0.0, t) == LookupStatus::ok);
  RC_CHECK_NEAR(t.x, 0.0, 1e-12);
  RC_CHECK(tree.lookup("odom", "base", 1.0, t) == LookupStatus::ok);
  RC_CHECK_NEAR(t.x, 0.6, 1e-12);
  RC_CHECK(tree.lookup("odom", "base", 0.25, t) == LookupStatus::ok);
  RC_CHECK_NEAR(t.x, 0.15, 1e-12);
}

RC_TEST("a frame not in the tree, two trees that do not meet, and a time outside what was reported are each refused for their own reason") {
  FrameTree tree;
  tree.set("base", "odom", 0.0, Transform2{});
  tree.set("base", "odom", 1.0, Transform2{1.0, 0.0, 0.0});
  tree.set("elsewhere", "other_root", 0.0, Transform2{});
  Transform2 t;
  const LookupStatus unknown = tree.lookup("odom", "nowhere", 0.5, t);
  const LookupStatus apart = tree.lookup("odom", "elsewhere", 0.5, t);
  const LookupStatus early = tree.lookup("odom", "base", -0.1, t);
  const LookupStatus late = tree.lookup("odom", "base", 1.5, t);
  std::cout << "  unknown frame: " << (unknown == LookupStatus::unknown_frame ? "unknown_frame" : "other") << "; two separate trees: "
            << (apart == LookupStatus::not_connected ? "not_connected" : "other") << "; before the first report: " << (early == LookupStatus::extrapolation ? "extrapolation" : "other")
            << "; after the last: " << (late == LookupStatus::extrapolation ? "extrapolation" : "other") << "\n\n";
  RC_CHECK(unknown == LookupStatus::unknown_frame);
  RC_CHECK(apart == LookupStatus::not_connected);
  RC_CHECK(early == LookupStatus::extrapolation);
  RC_CHECK(late == LookupStatus::extrapolation);
}

RC_TEST("a scan stamped a moment ago is placed with where the robot was then, and using where it is now puts every obstacle out by speed times delay") {
  FrameTree tree;
  const double speed = 0.6;
  for (int i = 0; i <= 20; ++i) tree.set("base", "odom", i * 0.1, Transform2{speed * i * 0.1, 0.0, 0.0});   // 2 s of driving along x
  tree.set_static("laser", "base", Transform2{0.0, 0.0, 0.0});
  const double scan_time = 1.0;
  const double now = 1.5;                                                              // the scan took half a second to arrive
  Transform2 then;
  Transform2 latest;
  RC_CHECK(tree.lookup("odom", "laser", scan_time, then) == LookupStatus::ok);
  RC_CHECK(tree.lookup("odom", "laser", now, latest) == LookupStatus::ok);
  double tx = 0.0;
  double ty = 0.0;
  double lx = 0.0;
  double ly = 0.0;
  apply(then, 2.0, 0.0, tx, ty);                                                       // a post 2 m ahead of the laser when the scan was taken
  apply(latest, 2.0, 0.0, lx, ly);
  std::cout << "  robot at 0.6 m/s, a scan stamped 0.5 s ago: post placed at x = " << std::fixed << std::setprecision(2) << tx << " with the transform at the stamp, " << lx
            << " with the latest; the error is " << lx - tx << " m\n\n";
  RC_CHECK_NEAR(tx, 2.6, 1e-9);
  RC_CHECK_NEAR(lx - tx, speed * (now - scan_time), 1e-9);
}
