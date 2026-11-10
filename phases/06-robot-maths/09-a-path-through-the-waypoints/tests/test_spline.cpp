#include <rc/test/rc_test.hpp>

#include <cmath>
#include <iomanip>
#include <iostream>
#include <vector>

#include "solution.hpp"

namespace {

std::vector<P> square() {
  std::vector<P> w;
  w.push_back(P{0, 0});
  w.push_back(P{10, 0});
  w.push_back(P{10, 10});
  w.push_back(P{0, 10});
  return w;
}

// A long run east, a short step north, and a long run back to the north west: the spacing is uneven, 10, 1 and 14.1.
std::vector<P> uneven() {
  std::vector<P> w;
  w.push_back(P{0, 0});
  w.push_back(P{10, 0});
  w.push_back(P{10, 1});
  w.push_back(P{0, 11});
  return w;
}

}  // namespace

RC_TEST("the curve passes through every waypoint, whatever the timing") {
  const P a{0, 0};
  const P b{4, 3};
  const P c{9, 1};
  const P d{12, 6};
  const double alphas[] = {0.0, 0.5, 1.0};
  for (double alpha : alphas) {
    const P start = catmull_rom_point(a, b, c, d, 0.0, alpha);
    const P end = catmull_rom_point(a, b, c, d, 1.0, alpha);
    RC_CHECK_NEAR(start.x, b.x, 1e-9);
    RC_CHECK_NEAR(start.y, b.y, 1e-9);
    RC_CHECK_NEAR(end.x, c.x, 1e-9);
    RC_CHECK_NEAR(end.y, c.y, 1e-9);
  }

  // And the sampled path ends exactly on the first and last waypoints.
  const std::vector<P> path = sample_path(square(), 10, 0.5);
  RC_CHECK_EQ(path.size(), static_cast<std::size_t>(3 * 10 + 1));
  RC_CHECK_NEAR(path.front().x, 0.0, 1e-12);
  RC_CHECK_NEAR(path.back().y, 10.0, 1e-12);
}

RC_TEST("waypoints on a straight line stay on it, and repeated waypoints are taken once") {
  std::vector<P> line;
  for (int i = 0; i < 6; ++i) line.push_back(P{static_cast<double>(i) * 2.0, 1.0});
  line.push_back(P{10.0, 1.0});          // repeated
  const double alphas[] = {0.0, 0.5, 1.0};
  for (double alpha : alphas) {
    const std::vector<P> path = sample_path(line, 8, alpha);
    double worst = 0.0;
    for (const P& p : path) worst = std::max(worst, std::fabs(p.y - 1.0));
    RC_CHECK_NEAR(worst, 0.0, 1e-9);
    RC_CHECK_NEAR(max_turn_degrees(path), 0.0, 1e-6);
    RC_CHECK_NEAR(path_length(path), 10.0, 1e-6);            // from 0 to 10 along x, no matter how the timing is spread
  }
  RC_CHECK_EQ(sample_path(line, 8, 0.5).size(), static_cast<std::size_t>(5 * 8 + 1));

  RC_CHECK(sample_path(std::vector<P>(), 8, 0.5).empty());
  RC_CHECK_EQ(sample_path(std::vector<P>(1, P{1, 1}), 8, 0.5).size(), static_cast<std::size_t>(1));
}

RC_TEST("the length of a path and its sharpest turn") {
  std::vector<P> right_angle;
  right_angle.push_back(P{0, 0});
  right_angle.push_back(P{3, 0});
  right_angle.push_back(P{3, 4});
  RC_CHECK_NEAR(path_length(right_angle), 7.0, 1e-12);
  RC_CHECK_NEAR(max_turn_degrees(right_angle), 90.0, 1e-9);

  std::vector<P> back;
  back.push_back(P{0, 0});
  back.push_back(P{2, 0});
  back.push_back(P{0, 0});
  RC_CHECK_NEAR(max_turn_degrees(back), 180.0, 1e-6);
  RC_CHECK_NEAR(path_length(std::vector<P>()), 0.0, 1e-12);
  RC_CHECK_NEAR(max_turn_degrees(std::vector<P>(2, P{1, 1})), 0.0, 1e-12);
}

RC_TEST("straight lines between waypoints turn through a full corner, and the curve does not") {
  const std::vector<P> w = square();
  const double polyline_turn = max_turn_degrees(w);
  const std::vector<P> curve = sample_path(w, 20, 0.5);
  const double curve_turn = max_turn_degrees(curve);
  std::cout << "  a square route: the straight lines turn " << std::fixed << std::setprecision(1) << polyline_turn
            << " degrees at a corner in one step, the curve at most " << curve_turn << " degrees in one of its steps; lengths "
            << std::setprecision(2) << path_length(w) << " and " << path_length(curve) << "\n\n";
  RC_CHECK_NEAR(polyline_turn, 90.0, 1e-9);
  RC_CHECK(curve_turn < 15.0);
  RC_CHECK(path_length(curve) < path_length(w) * 1.1);
}

RC_TEST("with uneven spacing the uniform curve loops, and the centripetal one does not") {
  const std::vector<P> w = uneven();
  std::cout << "  spacing 10, 1, 14.1:   alpha   sharpest turn in a step   path length\n";
  double turn_uniform = 0.0;
  double turn_centripetal = 0.0;
  const double alphas[] = {0.0, 0.5, 1.0};
  for (double alpha : alphas) {
    const std::vector<P> path = sample_path(w, 40, alpha);
    const double turn = max_turn_degrees(path);
    std::cout << "                         " << std::fixed << std::setprecision(1) << std::setw(5) << alpha << std::setw(22) << turn
              << std::setprecision(2) << std::setw(16) << path_length(path) << "\n";
    if (alpha == 0.0) turn_uniform = turn;
    if (alpha == 0.5) turn_centripetal = turn;
  }
  std::cout << "\n";
  RC_CHECK(turn_uniform > 120.0);                 // it turns back on itself: a loop
  RC_CHECK(turn_centripetal < 20.0);
  RC_CHECK(turn_uniform > 6.0 * turn_centripetal);
}
