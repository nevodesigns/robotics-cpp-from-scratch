#include <rc/test/rc_test.hpp>

#include <rc/math/vector.hpp>
#include <rc/nav/grid.hpp>
#include <rc/nav/plan.hpp>
#include <rc/nav/pursuit.hpp>
#include <rc/sim/diff_drive.hpp>

#include <cmath>
#include <cstdint>
#include <deque>
#include <iomanip>
#include <iostream>
#include <vector>

#include "solution.hpp"

namespace {

using rc::math::Vec2;
using rc::nav::PlanOptions;
using rc::nav::PurePursuit;
using rc::nav::wheels_for;
using rc::sim::Pose;

constexpr int kWidth = 80;
constexpr int kHeight = 60;
constexpr double kRes = 0.10;
constexpr double kWheelBase = 0.40;
constexpr double kDt = 0.02;

void set_free(OccupancyGrid& grid, int x, int y) {
  for (int i = 0; i < 12; ++i) grid.observe(x, y, false);
}

void set_blocked(OccupancyGrid& grid, int x, int y) {
  for (int i = 0; i < 12; ++i) grid.observe(x, y, true);
}

// A room with a wall across it and two ways through: a 0.40 m gap straight
// ahead and a 0.90 m gap a long way off to one side.
OccupancyGrid room_with_two_gaps() {
  OccupancyGrid grid(kWidth, kHeight, kRes, 0.0, 0.0);
  for (int y = 0; y < kHeight; ++y)
    for (int x = 0; x < kWidth; ++x) set_free(grid, x, y);
  for (int x = 0; x < kWidth; ++x) {
    set_blocked(grid, x, 0);
    set_blocked(grid, x, kHeight - 1);
  }
  for (int y = 0; y < kHeight; ++y) {
    set_blocked(grid, 0, y);
    set_blocked(grid, kWidth - 1, y);
  }
  for (int y = 1; y < kHeight - 1; ++y) {
    const bool narrow = y >= 29 && y <= 32;
    const bool wide = y >= 5 && y <= 19;
    if (!narrow && !wide) set_blocked(grid, 40, y);
  }
  return grid;
}

std::vector<Vec2> to_world(const std::vector<GridPoint>& cells, double resolution) {
  std::vector<Vec2> path;
  path.reserve(cells.size());
  for (const GridPoint& cell : cells)
    path.push_back({(static_cast<double>(cell.x) + 0.5) * resolution,
                    (static_cast<double>(cell.y) + 0.5) * resolution});
  return path;
}

struct DriveResult {
  bool arrived = false;
  double worst_clearance = 0.0;
  double travelled = 0.0;
};

// Drive a planned route with the follower from 16-04, steering by a pose that
// is `late` steps old, and measure how close the robot itself ever came to
// anything. Not the path: the robot.
DriveResult drive(const OccupancyGrid& truth, const std::vector<Vec2>& path,
                  double lookahead, double speed, int late) {
  PurePursuit follower(path, lookahead);
  Pose pose{path.front().x, path.front().y, 0.0};
  std::deque<Pose> history;

  DriveResult result;
  result.worst_clearance = 1e300;

  for (int i = 0; i < 6000; ++i) {
    history.push_back(pose);
    if (static_cast<int>(history.size()) > late + 1) history.pop_front();

    const auto steering = follower.follow(history.front());
    const auto wheels = wheels_for(speed, steering.curvature, kWheelBase);
    const Pose before = pose;
    pose = rc::sim::step(pose, wheels.left, wheels.right, kWheelBase, kDt);
    result.travelled += std::hypot(pose.x - before.x, pose.y - before.y);

    const double here = clearance_at(truth, pose.x, pose.y);
    if (here >= 0.0 && here < result.worst_clearance) result.worst_clearance = here;

    if (follower.arrived(pose, 0.15)) {
      result.arrived = true;
      break;
    }
  }
  return result;
}

}  // namespace

RC_TEST("inflation grows obstacles by a disc, not a square") {
  OccupancyGrid grid(21, 21, 0.10, 0.0, 0.0);
  for (int y = 0; y < 21; ++y)
    for (int x = 0; x < 21; ++x) set_free(grid, x, y);
  set_blocked(grid, 10, 10);

  // A radius of 0.25 m on a 10 cm grid is two and a half cells, and half cells
  // do not exist, so it rounds up to three. Inflation is conservative by up to
  // one cell and that has to be part of the margin rather than a surprise.
  const OccupancyGrid grown = inflated(grid, 0.25);

  RC_CHECK(grown.classify(13, 10) == Cell::occupied);   // three cells out
  RC_CHECK(grown.classify(14, 10) == Cell::free);       // four is clear

  // On the diagonal the disc stops sooner than a square would. Four cells
  // across and four up is 5.66 cells away, outside a radius of three, and a
  // square inflation of three would have blocked it: root two more area, and
  // gaps closed that the robot would have fitted through.
  RC_CHECK(grown.classify(13, 13) == Cell::free);
  RC_CHECK(grown.classify(12, 12) == Cell::occupied);   // 2.83 cells, inside

  // The single obstacle is still an obstacle.
  RC_CHECK(grown.classify(10, 10) == Cell::occupied);

  // Off the edge of the map counts as blocked, so the border grows inward.
  RC_CHECK(grown.classify(1, 1) == Cell::occupied);

  // Unknown grows as unknown rather than as occupied: a cell beside unseen
  // ground is not known to be blocked.
  OccupancyGrid partly(21, 21, 0.10, 0.0, 0.0);
  for (int y = 5; y < 16; ++y)
    for (int x = 5; x < 16; ++x) set_free(partly, x, y);
  const OccupancyGrid partly_grown = inflated(partly, 0.15);
  RC_CHECK(partly_grown.classify(10, 10) == Cell::free);
  RC_CHECK(partly_grown.classify(6, 10) == Cell::unknown);
}

RC_TEST("a robot with a width does not fit through a planner's shortest path") {
  const OccupancyGrid map = room_with_two_gaps();
  const GridPoint start{5, 30}, goal{74, 30};

  std::cout << "\n    a wall with a 0.40 m gap straight ahead and a 1.50 m gap\n";
  std::cout << "    twenty cells off to the side\n\n";
  std::cout << "    " << std::right << std::setw(14) << "robot radius" << std::setw(12)
            << "route" << std::setw(14) << "length m" << std::setw(18)
            << "clearance m" << "\n";

  double point_clearance = 0.0, disc_clearance = 0.0;
  double point_length = 0.0, disc_length = 0.0;
  bool wide_robot_found = true;

  for (const double radius : {0.00, 0.15, 0.25, 0.35, 0.50}) {
    const OccupancyGrid planning = radius > 0.0 ? inflated(map, radius) : map;
    const auto plan = rc::nav::plan_path(planning, start, goal, PlanOptions{});

    std::cout << "    " << std::right << std::fixed << std::setprecision(2)
              << std::setw(14) << radius << std::setw(12)
              << (plan.found ? "found" : "none");
    if (!plan.found) {
      if (radius >= 0.50) wide_robot_found = false;
      std::cout << "\n";
      continue;
    }

    const double length = rc::nav::path_length(plan.cells) * kRes;
    const double clearance = clearance_along(map, plan.cells);
    if (radius == 0.00) { point_clearance = clearance; point_length = length; }
    if (radius == 0.25) { disc_clearance = clearance; disc_length = length; }

    std::cout << std::setprecision(3) << std::setw(14) << length << std::setw(18)
              << clearance << "\n";
  }

  std::cout << "\n    the shortest route is shortest because it goes through the\n";
  std::cout << "    gap the robot does not fit through\n";

  // Treated as a point, the robot takes the near gap and passes 20 cm from the
  // wall, which a robot of 25 cm radius does by hitting it.
  RC_CHECK_NEAR(point_clearance, 0.20, 0.01);

  // Inflated for its own width, it takes the long way and clears.
  RC_CHECK(disc_clearance > 0.25);
  RC_CHECK(disc_length > point_length * 1.15);

  // And a robot too wide for either gap gets no route rather than a bad one.
  RC_CHECK(!wide_robot_found);
}

RC_TEST("clearance of the path is not clearance of the robot") {
  const OccupancyGrid map = room_with_two_gaps();
  const GridPoint start{5, 30}, goal{74, 30};
  const double radius = 0.25;

  const OccupancyGrid planning = inflated(map, radius);
  const auto plan = rc::nav::plan_path(planning, start, goal, PlanOptions{});
  RC_REQUIRE(plan.found);

  const std::vector<Vec2> path = to_world(plan.cells, kRes);
  const double planned = clearance_along(map, plan.cells);

  std::cout << "\n    driving the planned route, and what the robot itself\n";
  std::cout << "    actually cleared\n\n";
  std::cout << "    " << std::right << std::setw(12) << "lookahead" << std::setw(12)
            << "pose age" << std::setw(12) << "arrived" << std::setw(18)
            << "robot clearance" << "\n";
  std::cout << "    " << std::left << std::setw(24) << "the path itself"
            << std::right << std::fixed << std::setprecision(3) << planned << "\n";

  double tight = 0.0, loose = 0.0;
  for (const double lookahead : {0.30, 0.60, 1.00}) {
    const DriveResult driven = drive(map, path, lookahead, 0.6, 5);
    if (lookahead == 0.30) tight = driven.worst_clearance;
    if (lookahead == 1.00) loose = driven.worst_clearance;
    std::cout << "    " << std::right << std::fixed << std::setprecision(2)
              << std::setw(12) << lookahead << std::setw(12) << "0.10 s"
              << std::setw(12) << (driven.arrived ? "yes" : "no")
              << std::setprecision(3) << std::setw(18) << driven.worst_clearance << "\n";
  }

  std::cout << "\n    how much the robot clears is decided by the follower, not\n";
  std::cout << "    by the plan: at a metre of lookahead it passes closer than\n";
  std::cout << "    its own radius on a route planned with that radius in it\n";

  // A short lookahead follows the route closely enough that the robot clears
  // roughly what the route did. A long one does not.
  RC_CHECK(loose < tight);

  // And at a metre of lookahead the robot passes closer than its own radius,
  // on a route planned with that radius inflated in. This is the sum the lesson
  // is about: the margin has to cover the robot's width, the follower's corner
  // cut and the estimator's error, and only the first of the three is in the
  // inflation.
  RC_CHECK(loose < 0.25);
  RC_CHECK(loose < planned * 0.5);
}

RC_TEST("a map that changes under a plan that does not") {
  OccupancyGrid map = room_with_two_gaps();
  const GridPoint start{5, 30}, goal{74, 30};
  const double radius = 0.25;

  const auto first = rc::nav::plan_path(inflated(map, radius), start, goal, PlanOptions{});
  RC_REQUIRE(first.found);

  // A pallet is put down after the route was planned, where the route goes.
  int crossing_y = -1;
  for (const GridPoint& cell : first.cells)
    if (cell.x == 40) { crossing_y = cell.y; break; }
  RC_REQUIRE(crossing_y >= 0);
  for (int y = crossing_y - 2; y <= crossing_y + 2; ++y)
    for (int x = 38; x <= 42; ++x)
      if (map.inside(x, y)) set_blocked(map, x, y);

  // The old plan now runs straight through it, and nothing in the plan knows.
  int through_the_pallet = 0;
  for (const GridPoint& cell : first.cells)
    if (map.classify(cell.x, cell.y) == Cell::occupied) ++through_the_pallet;

  const auto second = rc::nav::plan_path(inflated(map, radius), start, goal, PlanOptions{});

  std::cout << "\n    a pallet is put down in the gap the route used\n\n";
  std::cout << "    " << std::left << std::setw(34) << "cells of the old plan now blocked"
            << std::right << through_the_pallet << "\n";
  std::cout << "    " << std::left << std::setw(34) << "replanned route found"
            << std::right << (second.found ? "yes" : "no") << "\n";
  if (second.found)
    std::cout << "    " << std::left << std::setw(34) << "replanned length m"
              << std::right << std::fixed << std::setprecision(3)
              << rc::nav::path_length(second.cells) * kRes << "\n";

  std::cout << "\n    a plan is a photograph of a map, and it is out of date the\n";
  std::cout << "    moment anything moves\n";

  RC_CHECK(through_the_pallet > 0);
  RC_REQUIRE(second.found);

  // The replanned route avoids it entirely.
  int still_blocked = 0;
  for (const GridPoint& cell : second.cells)
    if (map.classify(cell.x, cell.y) == Cell::occupied) ++still_blocked;
  RC_CHECK_EQ(still_blocked, 0);

  // Checking the current plan against the current map is far cheaper than
  // replanning, which is what makes it affordable every tick.
  RC_CHECK(second.expanded > static_cast<int>(first.cells.size()));
}
