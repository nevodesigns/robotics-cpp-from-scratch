#include <rc/test/rc_test.hpp>

#include <cmath>
#include <iomanip>
#include <iostream>
#include <vector>

#include "solution.hpp"

namespace {

const double kPi = 3.14159265358979323846;

// `n` points along a line at `degrees` through (1, 2), spread over a length of 10, with perpendicular noise.
std::vector<Point> along(double degrees, int n, double noise, rc::core::Rng& rng) {
  const double c = std::cos(degrees * kPi / 180.0);
  const double s = std::sin(degrees * kPi / 180.0);
  std::vector<Point> pts;
  for (int i = 0; i < n; ++i) {
    const double t = (rng.uniform() - 0.5) * 10.0;
    const double off = noise * rng.gaussian();
    pts.push_back(Point{1.0 + t * c - off * s, 2.0 + t * s + off * c});
  }
  return pts;
}

}  // namespace

RC_TEST("the distance from a point to a line is perpendicular and the angle ignores which way the line points") {
  Line horizontal;                 // y = 2: 0 x + 1 y - 2 = 0
  horizontal.a = 0.0;
  horizontal.b = 1.0;
  horizontal.c = -2.0;
  horizontal.valid = true;
  RC_CHECK_NEAR(distance_to_line(horizontal, Point{5.0, 5.0}), 3.0, 1e-12);
  RC_CHECK_NEAR(distance_to_line(horizontal, Point{-7.0, 2.0}), 0.0, 1e-12);
  RC_CHECK_NEAR(line_angle_degrees(horizontal), 0.0, 1e-9);

  Line flipped = horizontal;       // the same line, described from the other side
  flipped.a = -flipped.a;
  flipped.b = -flipped.b;
  flipped.c = -flipped.c;
  RC_CHECK_NEAR(line_angle_degrees(flipped), 0.0, 1e-9);

  Line vertical;                   // x = 3
  vertical.a = 1.0;
  vertical.b = 0.0;
  vertical.c = -3.0;
  vertical.valid = true;
  RC_CHECK_NEAR(line_angle_degrees(vertical), 90.0, 1e-9);
  RC_CHECK_NEAR(distance_to_line(vertical, Point{5.0, 100.0}), 2.0, 1e-12);

  RC_CHECK_NEAR(angle_difference_degrees(179.0, 1.0), 2.0, 1e-12);
  RC_CHECK_NEAR(angle_difference_degrees(10.0, 40.0), 30.0, 1e-12);

  // Two points are a line, and it is the one that passes through both.
  std::vector<Point> two;
  two.push_back(Point{0.0, 1.0});
  two.push_back(Point{4.0, 1.0});
  const Line through = fit_line(two);
  RC_CHECK(through.valid);
  RC_CHECK_NEAR(distance_to_line(through, Point{2.0, 1.0}), 0.0, 1e-12);
  RC_CHECK_NEAR(distance_to_line(through, Point{2.0, 3.0}), 2.0, 1e-12);
}

RC_TEST("exact points give the exact line, including a line that goes straight up") {
  std::vector<Point> flat;
  for (int i = 0; i < 6; ++i) flat.push_back(Point{static_cast<double>(i), 2.0});
  const Line f = fit_line(flat);
  RC_CHECK(f.valid);
  RC_CHECK_NEAR(line_angle_degrees(f), 0.0, 1e-9);

  std::vector<Point> diagonal;
  for (int i = 0; i < 6; ++i) diagonal.push_back(Point{static_cast<double>(i), static_cast<double>(i) + 1.0});
  const Line d = fit_line(diagonal);
  RC_CHECK(d.valid);
  RC_CHECK_NEAR(line_angle_degrees(d), 45.0, 1e-9);
  RC_CHECK_NEAR(distance_to_line(d, Point{3.0, 4.0}), 0.0, 1e-9);

  std::vector<Point> up;
  for (int i = 0; i < 6; ++i) up.push_back(Point{3.0, static_cast<double>(i)});
  const Line u = fit_line(up);
  const Line naive = fit_vertical_distances(up);
  std::cout << "  points on x = 3: fit_line gives a line at " << std::fixed << std::setprecision(1) << line_angle_degrees(u)
            << " degrees, valid; the vertical distance fit is " << (naive.valid ? "valid" : "not valid (a vertical line has no slope)") << "\n\n";
  RC_CHECK(u.valid);
  RC_CHECK_NEAR(line_angle_degrees(u), 90.0, 1e-9);
  RC_CHECK_NEAR(distance_to_line(u, Point{3.0, 100.0}), 0.0, 1e-9);
  RC_CHECK(!naive.valid);

  RC_CHECK(!fit_line(std::vector<Point>()).valid);
  RC_CHECK(!fit_line(std::vector<Point>(1, Point{1.0, 1.0})).valid);
  RC_CHECK(!fit_line(std::vector<Point>(5, Point{2.0, 2.0})).valid);       // all one point: no direction
}

RC_TEST("on a steep line the vertical distance fit is worse than the perpendicular one") {
  const int runs = 300;
  double sum_vertical = 0.0;
  double sum_perpendicular = 0.0;
  for (int seed = 1; seed <= runs; ++seed) {
    rc::core::Rng rng(static_cast<unsigned>(seed));
    const std::vector<Point> pts = along(85.0, 30, 0.1, rng);
    const Line v = fit_vertical_distances(pts);
    const Line p = fit_line(pts);
    sum_vertical += v.valid ? angle_difference_degrees(line_angle_degrees(v), 85.0) : 90.0;
    sum_perpendicular += angle_difference_degrees(line_angle_degrees(p), 85.0);
  }
  std::cout << "  a line at 85 degrees, 30 points with noise 0.1, " << runs << " runs: mean error of the angle "
            << std::fixed << std::setprecision(2) << sum_vertical / runs << " degrees with vertical distances, " << sum_perpendicular / runs
            << " with perpendicular\n";
  RC_CHECK(sum_perpendicular / runs < 1.0);
  RC_CHECK(sum_vertical / runs > 2.0 * sum_perpendicular / runs);
}

RC_TEST("thirty wrong points among a hundred pull an ordinary fit far off, and a random-sample fit ignores them") {
  const int runs = 200;
  double sum_all = 0.0;
  double sum_ransac = 0.0;
  int worst_ransac_inliers = 1000;
  for (int seed = 1; seed <= runs; ++seed) {
    rc::core::Rng rng(static_cast<unsigned>(seed));
    std::vector<Point> pts = along(30.0, 100, 0.02, rng);
    // The wrong points are a cluster, such as a second object seen by the same sensor, and not spread evenly.
    for (int i = 0; i < 30; ++i) pts.push_back(Point{2.0 + 4.0 * rng.uniform(), 7.0 + 3.0 * rng.uniform()});

    const Line all = fit_line(pts);
    const Line robust = ransac_line(pts, 100, 0.08, static_cast<unsigned>(seed) * 7u + 1u);
    sum_all += all.valid ? angle_difference_degrees(line_angle_degrees(all), 30.0) : 90.0;
    sum_ransac += robust.valid ? angle_difference_degrees(line_angle_degrees(robust), 30.0) : 90.0;

    int inliers = 0;
    for (std::size_t i = 0; i < 100; ++i) {
      if (robust.valid && distance_to_line(robust, pts[i]) <= 0.1) ++inliers;
    }
    if (inliers < worst_ransac_inliers) worst_ransac_inliers = inliers;
  }
  std::cout << "  a line at 30 degrees, 100 points on it and 30 wrong ones, " << runs << " runs: mean error of the angle "
            << std::fixed << std::setprecision(2) << sum_all / runs << " degrees fitting everything, " << sum_ransac / runs
            << " with the random-sample fit; it kept at least " << worst_ransac_inliers << " of the 100 true points close to the line\n\n";
  RC_CHECK(sum_all / runs > 3.0);
  RC_CHECK(sum_ransac / runs < 0.5);
  RC_CHECK(worst_ransac_inliers >= 90);
}

RC_TEST("the random-sample fit gives the same line for the same seed, and says so when it has nothing to fit") {
  rc::core::Rng rng(5);
  std::vector<Point> pts = along(60.0, 50, 0.02, rng);
  for (int i = 0; i < 10; ++i) pts.push_back(Point{rng.uniform() * 10.0, rng.uniform() * 10.0});
  const Line a = ransac_line(pts, 50, 0.08, 99);
  const Line b = ransac_line(pts, 50, 0.08, 99);
  RC_CHECK(a.valid);
  RC_CHECK_NEAR(a.a, b.a, 1e-15);
  RC_CHECK_NEAR(a.c, b.c, 1e-15);
  RC_CHECK_NEAR(line_angle_degrees(a), 60.0, 1.0);

  RC_CHECK(!ransac_line(std::vector<Point>(), 10, 0.1, 1).valid);
  RC_CHECK(!ransac_line(std::vector<Point>(1, Point{1.0, 1.0}), 10, 0.1, 1).valid);
}
