#include <rc/test/rc_test.hpp>

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <vector>

#include "solution.hpp"

namespace {

V v(double x, double y) { return V{x, y}; }

// The obvious ray casting test: count every edge whose height range includes the ray, ends included.
bool naive_inside(const V& p, const std::vector<V>& poly) {
  bool inside = false;
  const std::size_t n = poly.size();
  for (std::size_t i = 0, j = n - 1; i < n; j = i++) {
    const V& a = poly[j];
    const V& b = poly[i];
    if (a.y == b.y) continue;
    if (std::min(a.y, b.y) <= p.y && p.y <= std::max(a.y, b.y)) {
      const double x_at_ray = a.x + (p.y - a.y) * (b.x - a.x) / (b.y - a.y);
      if (p.x < x_at_ray) inside = !inside;
    }
  }
  return inside;
}

// A diamond with corners on the axes: inside is exactly |x| + |y| < 1, with the boundary |x| + |y| = 1.
std::vector<V> diamond() {
  std::vector<V> d;
  d.push_back(v(1, 0));
  d.push_back(v(0, 1));
  d.push_back(v(-1, 0));
  d.push_back(v(0, -1));
  return d;
}

}  // namespace

RC_TEST("which side of a line a point is on, and a line three decimal numbers cannot quite draw") {
  RC_CHECK_EQ(orientation(v(0, 0), v(4, 0), v(2, 3)), 1);        // above a line going right: on the left
  RC_CHECK_EQ(orientation(v(0, 0), v(4, 0), v(2, -3)), -1);
  RC_CHECK_EQ(orientation(v(0, 0), v(4, 0), v(9, 0)), 0);

  // Three points on y = 3x that the computer cannot store exactly: on paper they are in a line.
  const V a = v(0.1, 0.3);
  const V b = v(0.7, 2.1);
  const V c = v(1.1, 3.3);
  const double raw = cross(a, b, c);
  std::cout << "  (0.1, 0.3), (0.7, 2.1), (1.1, 3.3) are on a line on paper. cross() says " << std::scientific << std::setprecision(2) << raw
            << ", orientation() says " << orientation(a, b, c) << "\n\n";
  RC_CHECK(raw != 0.0);                                            // the raw number is not zero
  RC_CHECK_EQ(orientation(a, b, c), 0);                            // and the test that allows for rounding says they are in a line
  RC_CHECK_EQ(orientation(v(0, 0), v(1, 0), v(2, 1e-6)), 1);       // a point that really is off the line is not mistaken for on it
}

RC_TEST("segments that cross, touch, overlap, miss and never meet") {
  RC_CHECK(segments_intersect(v(0, 0), v(4, 4), v(0, 4), v(4, 0)));       // an X
  RC_CHECK(segments_intersect(v(0, 0), v(4, 0), v(2, 0), v(2, 3)));       // a T: an end on the other
  RC_CHECK(segments_intersect(v(0, 0), v(2, 2), v(2, 2), v(5, 0)));       // end to end
  RC_CHECK(segments_intersect(v(0, 0), v(4, 0), v(2, 0), v(6, 0)));       // on one line, overlapping
  RC_CHECK(segments_intersect(v(0, 0), v(4, 0), v(1, 0), v(2, 0)));       // one inside the other
  RC_CHECK(!segments_intersect(v(0, 0), v(2, 0), v(3, 0), v(6, 0)));      // on one line, a gap between: the case a quick test gets wrong
  RC_CHECK(!segments_intersect(v(0, 0), v(4, 0), v(0, 1), v(4, 1)));      // parallel
  RC_CHECK(!segments_intersect(v(0, 0), v(2, 2), v(3, 0), v(3, 5)));      // the lines cross, the segments do not reach
  RC_CHECK(!segments_intersect(v(0, 0), v(4, 4), v(5, 5), v(6, 6)));      // on one line, apart, diagonal
}

RC_TEST("the nearest point on a segment is the foot of the perpendicular or an end") {
  const V a = v(0, 0);
  const V b = v(10, 0);
  const V foot = closest_point_on_segment(v(4, 3), a, b);
  RC_CHECK_NEAR(foot.x, 4.0, 1e-12);
  RC_CHECK_NEAR(foot.y, 0.0, 1e-12);
  const V before = closest_point_on_segment(v(-5, 2), a, b);
  RC_CHECK_NEAR(before.x, 0.0, 1e-12);                          // past the start: the start
  const V after = closest_point_on_segment(v(14, -1), a, b);
  RC_CHECK_NEAR(after.x, 10.0, 1e-12);
  RC_CHECK_NEAR(distance_to_segment(v(4, 3), a, b), 3.0, 1e-12);
  RC_CHECK_NEAR(distance_to_segment(v(13, 4), a, b), 5.0, 1e-12);   // 3 along, 4 across, to the end
  RC_CHECK_NEAR(distance_to_segment(v(3, 4), v(1, 1), v(1, 1)), std::hypot(2.0, 3.0), 1e-12);   // a segment of no length is a point
}

RC_TEST("a point in a polygon, with the ray going exactly through corners") {
  const std::vector<V> d = diamond();
  RC_CHECK(point_in_polygon(v(0, 0), d));
  RC_CHECK(point_in_polygon(v(0.4, 0.4), d));
  RC_CHECK(!point_in_polygon(v(0.6, 0.6), d));
  RC_CHECK(!point_in_polygon(v(2, 0), d));
  RC_CHECK(point_in_polygon(v(1, 0), d));                       // on a corner counts as inside
  RC_CHECK(point_in_polygon(v(0.5, 0.5), d));                   // on an edge too

  // Every point of a grid, 0.25 apart, of which many have a ray that passes through the corners at (+-1, 0) and (0, +-1).
  int grid_points = 0;
  int naive_wrong = 0;
  int correct_wrong = 0;
  for (int i = -12; i <= 12; ++i) {
    for (int j = -12; j <= 12; ++j) {
      const V p = v(0.25 * i, 0.25 * j);
      const bool truth = std::fabs(p.x) + std::fabs(p.y) <= 1.0;       // inside or on the boundary
      ++grid_points;
      if (naive_inside(p, d) != truth) ++naive_wrong;
      if (point_in_polygon(p, d) != truth) ++correct_wrong;
    }
  }
  std::cout << "  a diamond and " << grid_points << " grid points: the obvious ray test is wrong for " << naive_wrong
            << " of them, the half-open one for " << correct_wrong << "\n\n";
  RC_CHECK_EQ(correct_wrong, 0);
  RC_CHECK(naive_wrong > 0);

  RC_CHECK(!point_in_polygon(v(0, 0), std::vector<V>()));
  RC_CHECK(!point_in_polygon(v(0, 0), std::vector<V>(2, v(1, 1))));
}

RC_TEST("the area of a polygon, and which way round its corners go") {
  std::vector<V> square;
  square.push_back(v(0, 0));
  square.push_back(v(4, 0));
  square.push_back(v(4, 3));
  square.push_back(v(0, 3));
  RC_CHECK_NEAR(polygon_area(square), 12.0, 1e-12);                       // anticlockwise: positive
  std::reverse(square.begin(), square.end());
  RC_CHECK_NEAR(polygon_area(square), -12.0, 1e-12);                      // clockwise: negative

  RC_CHECK_NEAR(polygon_area(diamond()), 2.0, 1e-12);

  std::vector<V> l_shape;                                                 // an L: a 4 by 4 square with a 2 by 2 corner cut out
  l_shape.push_back(v(0, 0));
  l_shape.push_back(v(4, 0));
  l_shape.push_back(v(4, 2));
  l_shape.push_back(v(2, 2));
  l_shape.push_back(v(2, 4));
  l_shape.push_back(v(0, 4));
  RC_CHECK_NEAR(polygon_area(l_shape), 12.0, 1e-12);
  RC_CHECK_NEAR(polygon_area(std::vector<V>()), 0.0, 1e-12);
}
