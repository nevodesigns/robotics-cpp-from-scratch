#include <rc/test/rc_test.hpp>

#include <rc/sim/checks.hpp>
#include <rc/sim/diff_drive.hpp>

#include <cmath>
#include <iostream>
#include <string>
#include <vector>

#include "solution.hpp"

namespace {

using rc::sim::Pose;

constexpr int kColumns = 60;
constexpr int kRows = 20;
constexpr double kWheelBase = 0.3;
constexpr double kDt = 0.02;
constexpr double kLeftSpeed = 0.9;
constexpr double kRightSpeed = 1.1;
constexpr double kQuarterTurn = 1.57079632679489661923;

Pose at(double x, double y) {
  Pose pose;
  pose.x = x;
  pose.y = y;
  pose.theta = 0.0;
  return pose;
}

PlotBounds bounds_of(const std::vector<Pose>& path) {
  PlotBounds bounds;
  for (const Pose& pose : path) bounds = include(bounds, pose);
  return bounds;
}

// The drawing itself, which the lesson supplies. The arithmetic above it is the
// part worth writing, and it is the part that is wrong when a plot is wrong.
std::string draw(const std::vector<Pose>& path, int columns, int rows) {
  if (path.empty()) return "  (nothing to draw)\n";

  const PlotBounds bounds = bounds_of(path);
  const double scale = scale_to_fit(bounds, columns, rows);

  std::vector<std::string> grid(static_cast<std::size_t>(rows),
                                std::string(static_cast<std::size_t>(columns), ' '));

  for (const Pose& pose : path) {
    const int column = column_for(pose.x, bounds, scale);
    const int row = row_for(pose.y, bounds, scale, rows);
    if (column < 0 || column >= columns || row < 0 || row >= rows) continue;
    grid[static_cast<std::size_t>(row)][static_cast<std::size_t>(column)] = '.';
  }

  const int start_column = column_for(path.front().x, bounds, scale);
  const int start_row = row_for(path.front().y, bounds, scale, rows);
  if (start_column >= 0 && start_column < columns && start_row >= 0 && start_row < rows)
    grid[static_cast<std::size_t>(start_row)][static_cast<std::size_t>(start_column)] = 'S';

  const int end_column = column_for(path.back().x, bounds, scale);
  const int end_row = row_for(path.back().y, bounds, scale, rows);
  if (end_column >= 0 && end_column < columns && end_row >= 0 && end_row < rows)
    grid[static_cast<std::size_t>(end_row)][static_cast<std::size_t>(end_column)] = 'E';

  std::string out;
  for (const std::string& line : grid) out += "  " + line + "\n";
  return out;
}

}  // namespace

RC_TEST("an empty plot has no extent, and knows it") {
  const PlotBounds bounds;
  RC_CHECK(bounds.empty);
  RC_CHECK_NEAR(bounds_width(bounds), 0.0, 1e-12);
  RC_CHECK_NEAR(bounds_height(bounds), 0.0, 1e-12);
}

RC_TEST("the first pose seeds the rectangle rather than being compared to the origin") {
  // The check that catches a bounds starting at zero. This path never goes near
  // the origin, and a plot that includes it anyway draws the path tiny and in a
  // corner.
  PlotBounds bounds;
  bounds = include(bounds, at(10.0, 20.0));
  bounds = include(bounds, at(12.0, 24.0));

  RC_CHECK_NEAR(bounds.min_x, 10.0, 1e-12);
  RC_CHECK_NEAR(bounds.min_y, 20.0, 1e-12);
  RC_CHECK_NEAR(bounds_width(bounds), 2.0, 1e-12);
  RC_CHECK_NEAR(bounds_height(bounds), 4.0, 1e-12);
}

RC_TEST("the rectangle grows in every direction") {
  PlotBounds bounds;
  bounds = include(bounds, at(0.0, 0.0));
  bounds = include(bounds, at(-5.0, 3.0));
  bounds = include(bounds, at(2.0, -7.0));

  RC_CHECK_NEAR(bounds.min_x, -5.0, 1e-12);
  RC_CHECK_NEAR(bounds.max_x, 2.0, 1e-12);
  RC_CHECK_NEAR(bounds.min_y, -7.0, 1e-12);
  RC_CHECK_NEAR(bounds.max_y, 3.0, 1e-12);
}

RC_TEST("a point higher up the field is drawn nearer the top") {
  // The check that catches a missing flip. A robot's y grows upward and a
  // screen's rows grow downward, and a mirror image looks plausible.
  PlotBounds bounds;
  bounds = include(bounds, at(0.0, 0.0));
  bounds = include(bounds, at(1.0, 1.0));
  const double scale = scale_to_fit(bounds, kColumns, kRows);

  const int low = row_for(0.0, bounds, scale, kRows);
  const int high = row_for(1.0, bounds, scale, kRows);

  RC_CHECK(high < low);
  RC_CHECK_EQ(low, kRows - 1);   // the bottom of the path is the bottom row
  RC_CHECK_EQ(high, 0);          // and the top of it is the top row
}

RC_TEST("a point further right is drawn further right") {
  PlotBounds bounds;
  bounds = include(bounds, at(0.0, 0.0));
  bounds = include(bounds, at(1.0, 1.0));
  const double scale = scale_to_fit(bounds, kColumns, kRows);

  RC_CHECK(column_for(1.0, bounds, scale) > column_for(0.0, bounds, scale));
  RC_CHECK_EQ(column_for(0.0, bounds, scale), 0);
}

RC_TEST("every point of a path lands inside the grid") {
  // The off by one lives here. A point at the very edge that maps to the row
  // index equal to the height is one past the end of the grid.
  std::vector<Pose> path;
  for (int i = 0; i <= 40; ++i) {
    const double angle = static_cast<double>(i) * 0.15;
    path.push_back(at(3.0 * std::cos(angle) + 7.0, 3.0 * std::sin(angle) - 2.0));
  }

  const PlotBounds bounds = bounds_of(path);
  const double scale = scale_to_fit(bounds, kColumns, kRows);

  for (const Pose& pose : path) {
    const int column = column_for(pose.x, bounds, scale);
    const int row = row_for(pose.y, bounds, scale, kRows);
    RC_REQUIRE(column >= 0);
    RC_REQUIRE(column < kColumns);
    RC_REQUIRE(row >= 0);
    RC_REQUIRE(row < kRows);
  }
}

RC_TEST("a circle is drawn round, not stretched to fill the grid") {
  // The check that catches two scales, one per axis. Stretching a path to fill
  // the grid makes a circle into an ellipse, and then the shape of the picture
  // has stopped being evidence about the robot.
  //
  // Round in a terminal means twice as wide as it is tall in characters,
  // because a character is about twice as tall as it is wide.
  std::vector<Pose> path;
  for (int i = 0; i <= 200; ++i) {
    const double angle = static_cast<double>(i) * 0.0314159;
    path.push_back(at(std::cos(angle), std::sin(angle)));
  }

  const PlotBounds bounds = bounds_of(path);
  const double scale = scale_to_fit(bounds, kColumns, kRows);

  int min_column = kColumns, max_column = -1, min_row = kRows, max_row = -1;
  for (const Pose& pose : path) {
    const int column = column_for(pose.x, bounds, scale);
    const int row = row_for(pose.y, bounds, scale, kRows);
    if (column < min_column) min_column = column;
    if (column > max_column) max_column = column;
    if (row < min_row) min_row = row;
    if (row > max_row) max_row = row;
  }

  const double drawn_width = static_cast<double>(max_column - min_column);
  const double drawn_height = static_cast<double>(max_row - min_row);
  RC_REQUIRE(drawn_height > 0.0);

  // Twice as wide as tall, within one character of rounding.
  RC_CHECK_NEAR(drawn_width / drawn_height, character_aspect(), 0.2);
}

RC_TEST("a path with no height still produces a picture rather than nothing") {
  // A robot driving in a straight line along x has zero extent in y. Dividing
  // by that gives infinity, every coordinate becomes not a number, and the plot
  // comes out blank with nothing to say why.
  PlotBounds bounds;
  bounds = include(bounds, at(0.0, 5.0));
  bounds = include(bounds, at(4.0, 5.0));

  const double scale = scale_to_fit(bounds, kColumns, kRows);
  RC_REQUIRE(std::isfinite(scale));

  for (double x = 0.0; x <= 4.0; x += 1.0) {
    const int column = column_for(x, bounds, scale);
    const int row = row_for(5.0, bounds, scale, kRows);
    RC_CHECK(column >= 0 && column < kColumns);
    RC_CHECK(row >= 0 && row < kRows);
  }
}

RC_TEST("a single point does not divide by zero in either direction") {
  PlotBounds bounds;
  bounds = include(bounds, at(2.0, 3.0));

  const double scale = scale_to_fit(bounds, kColumns, kRows);
  RC_REQUIRE(std::isfinite(scale));
  RC_CHECK(std::isfinite(static_cast<double>(column_for(2.0, bounds, scale))));
  RC_CHECK(std::isfinite(static_cast<double>(row_for(3.0, bounds, scale, kRows))));
}

RC_TEST("the robot from lesson 00-05, drawn") {
  // The point of the phase. Everything before this produced numbers, and a
  // number is hard to disbelieve. A picture is not.
  const Pose start = at(0.0, 0.0);

  // drive returns the poses it stepped to, so the starting pose is put back at
  // the front. Otherwise the S marks the first step rather than the start,
  // which is a small lie in a picture whose whole job is not to lie.
  // A quarter turn, worked out rather than guessed at. The turn rate is the
  // wheel difference over the wheel base, so the time for a quarter turn is a
  // quarter of a full one divided by that, and the step count follows.
  const double turn_rate = (kRightSpeed - kLeftSpeed) / kWheelBase;
  const int steps = static_cast<int>((kQuarterTurn / turn_rate) / kDt + 0.5);

  std::vector<Pose> path{start};
  const std::vector<Pose> driven =
      rc::sim::drive(start, kLeftSpeed, kRightSpeed, kWheelBase, kDt, steps);
  path.insert(path.end(), driven.begin(), driven.end());

  std::cout << "\n  a quarter circle, driven with the right wheel a little faster\n\n"
            << draw(path, kColumns, kRows) << "\n";

  // The picture is for the reader. These are for the suite: the path did turn,
  // it did move, and it stayed in one piece.
  RC_REQUIRE_EQ(path.size(), static_cast<std::size_t>(steps + 1));

  // It turned a quarter of a circle, which is what the caption says, checked
  // with the heading difference from the previous lesson so that pi and minus
  // pi do not disagree with each other.
  RC_CHECK_NEAR(rc::sim::heading_difference(start.theta, path.back().theta),
                kQuarterTurn, 0.02);
  RC_CHECK(!rc::sim::same_position(path.back(), start, 0.1));

  const PlotBounds bounds = bounds_of(path);
  RC_CHECK(bounds_width(bounds) > 0.1);
  RC_CHECK(bounds_height(bounds) > 0.1);
}

RC_TEST("a straight line and a curve do not look the same") {
  // What the picture is for. Two paths that end in almost the same place, one
  // of which took a very different route, and the numbers at the end barely
  // separate them.
  const Pose start = at(0.0, 0.0);
  const std::vector<Pose> straight = rc::sim::drive(start, 1.0, 1.0, kWheelBase, kDt, 200);
  const std::vector<Pose> curved = rc::sim::drive(start, 0.8, 1.2, kWheelBase, kDt, 200);

  std::cout << "  both wheels the same speed\n\n" << draw(straight, kColumns, 7) << "\n"
            << "  the right wheel half again as fast\n\n" << draw(curved, kColumns, 7) << "\n";

  RC_CHECK(!rc::sim::same_position(straight.back(), curved.back(), 0.01));
}
