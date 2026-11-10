#include <rc/test/rc_test.hpp>

#include <cmath>
#include <iomanip>
#include <iostream>
#include <limits>
#include <vector>

#include "solution.hpp"

namespace {

const double kSx = 2.0;
const double kSy = 1.0;
const double kRho = 0.8;

}  // namespace

RC_TEST("the covariance of a few points, worked out by hand") {
  std::vector<Pt> pts;
  pts.push_back(Pt{1.0, 2.0});
  pts.push_back(Pt{3.0, 6.0});
  pts.push_back(Pt{5.0, 10.0});
  // mean (3, 6); x offsets -2, 0, 2; y offsets -4, 0, 4; sums 8, 16, 32 over (3 - 1)
  const Cov2 c = covariance(pts);
  RC_CHECK_NEAR(c.xx, 4.0, 1e-12);
  RC_CHECK_NEAR(c.xy, 8.0, 1e-12);
  RC_CHECK_NEAR(c.yy, 16.0, 1e-12);

  const Cov2 none = covariance(std::vector<Pt>(1, Pt{1.0, 1.0}));
  RC_CHECK_NEAR(none.xx, 0.0, 1e-15);
  RC_CHECK_NEAR(covariance(std::vector<Pt>()).yy, 0.0, 1e-15);
}

RC_TEST("the long axis of a matrix, its two spreads and its direction, in closed form") {
  Cov2 diagonal;
  diagonal.xx = 9.0;
  diagonal.yy = 1.0;
  const Axes a = principal_axes(diagonal);
  RC_CHECK_NEAR(a.major, 9.0, 1e-12);
  RC_CHECK_NEAR(a.minor, 1.0, 1e-12);
  RC_CHECK_NEAR(a.angle_degrees, 0.0, 1e-9);

  Cov2 tall;                       // more spread in y than in x: the long axis points up
  tall.xx = 1.0;
  tall.yy = 9.0;
  RC_CHECK_NEAR(principal_axes(tall).angle_degrees, 90.0, 1e-9);

  Cov2 tilted;                     // [[2, 1], [1, 2]]: eigenvalues 3 and 1 along 45 degrees
  tilted.xx = 2.0;
  tilted.xy = 1.0;
  tilted.yy = 2.0;
  const Axes t = principal_axes(tilted);
  RC_CHECK_NEAR(t.major, 3.0, 1e-12);
  RC_CHECK_NEAR(t.minor, 1.0, 1e-12);
  RC_CHECK_NEAR(t.angle_degrees, 45.0, 1e-9);

  tilted.xy = -1.0;                // the other way: 135 degrees
  RC_CHECK_NEAR(principal_axes(tilted).angle_degrees, 135.0, 1e-9);
}

RC_TEST("a big sample of a tilted cloud has the covariance it was made with and the axis it should") {
  const std::vector<Pt> cloud = correlated_cloud(11, 20000, 5.0, -3.0, kSx, kSy, kRho);
  const Pt m = mean_of(cloud);
  const Cov2 c = covariance(cloud);
  const Axes a = principal_axes(c);
  const double true_xy = kRho * kSx * kSy;
  std::cout << "  20000 points made with spreads 2 and 1 and correlation 0.8:\n"
            << "    mean (" << std::fixed << std::setprecision(3) << m.x << ", " << m.y << ")   variances " << std::setprecision(2) << c.xx
            << " and " << c.yy << " (true 4 and 1), together " << c.xy << " (true " << true_xy << ")\n"
            << "    long axis " << std::setprecision(1) << a.angle_degrees << " degrees, spreads " << std::setprecision(2) << std::sqrt(a.major)
            << " and " << std::sqrt(a.minor) << "\n";
  RC_CHECK_NEAR(m.x, 5.0, 0.06);
  RC_CHECK_NEAR(m.y, -3.0, 0.03);
  RC_CHECK_NEAR(c.xx, 4.0, 0.15);
  RC_CHECK_NEAR(c.yy, 1.0, 0.04);
  RC_CHECK_NEAR(c.xy, true_xy, 0.06);
  RC_CHECK(a.angle_degrees > 20.0 && a.angle_degrees < 30.0);
}

RC_TEST("the ellipse holds the share of points it should, and the box does not match it") {
  const std::vector<Pt> cloud = correlated_cloud(12, 20000, 0.0, 0.0, kSx, kSy, kRho);
  const Pt m = mean_of(cloud);
  const Cov2 c = covariance(cloud);

  int in_ellipse = 0;
  int in_box = 0;
  for (const Pt& p : cloud) {
    if (inside_ellipse(p, m, c, kInside95)) ++in_ellipse;
    if (std::fabs(p.x - m.x) <= 2.0 * kSx && std::fabs(p.y - m.y) <= 2.0 * kSy) ++in_box;
  }
  const double share_ellipse = static_cast<double>(in_ellipse) / static_cast<double>(cloud.size());
  const double share_box = static_cast<double>(in_box) / static_cast<double>(cloud.size());

  const Axes a = principal_axes(c);
  const double ellipse_area = 3.14159265358979323846 * kInside95 * std::sqrt(a.major * a.minor);
  const double box_area = 16.0 * kSx * kSy;
  std::cout << "  the 95 percent ellipse held " << std::setprecision(1) << 100.0 * share_ellipse << " percent of the points and had an area of "
            << std::setprecision(1) << ellipse_area << "; a box of two spreads each way held " << 100.0 * share_box
            << " percent and had an area of " << box_area << "\n";
  RC_CHECK_NEAR(share_ellipse, 0.95, 0.01);
  RC_CHECK(share_box < share_ellipse);
  RC_CHECK(box_area > 1.3 * ellipse_area);
}

RC_TEST("a point can be close in metres and far in spreads, and the other way round") {
  const Pt centre{0.0, 0.0};
  Cov2 c;                          // spreads 2 and 1 with a correlation of 0.8
  c.xx = kSx * kSx;
  c.yy = kSy * kSy;
  c.xy = kRho * kSx * kSy;

  const Pt along{2.0, 0.8};         // along the direction the cloud is stretched
  const Pt across{1.2, -1.6};      // about as far from the centre in metres, but across the stretch
  const double d_along = mahalanobis_squared(along, centre, c);
  const double d_across = mahalanobis_squared(across, centre, c);
  std::cout << "  a point " << std::hypot(along.x, along.y) << " metres out along the cloud: squared distance " << std::fixed
            << std::setprecision(2) << d_along << "; a point " << std::hypot(across.x, across.y)
            << " metres out across it: " << d_across << "\n";
  RC_CHECK(std::hypot(across.x, across.y) > 0.9 * std::hypot(along.x, along.y));
  RC_CHECK(d_along < kInside95);
  RC_CHECK(d_across > 2.0 * kInside95);
  RC_CHECK(inside_ellipse(along, centre, c, kInside95));
  RC_CHECK(!inside_ellipse(across, centre, c, kInside95));

  // A point one spread away along an axis of an uncorrelated cloud has a squared distance of 1.
  Cov2 plain;
  plain.xx = 9.0;
  plain.yy = 4.0;
  RC_CHECK_NEAR(mahalanobis_squared(Pt{3.0, 0.0}, centre, plain), 1.0, 1e-12);
  RC_CHECK_NEAR(mahalanobis_squared(Pt{0.0, 2.0}, centre, plain), 1.0, 1e-12);
}

RC_TEST("a cloud with no width in one direction has no distance for a point off its line") {
  std::vector<Pt> line;
  for (int i = 0; i < 10; ++i) line.push_back(Pt{static_cast<double>(i), 2.0 * static_cast<double>(i)});
  const Pt m = mean_of(line);
  const Cov2 c = covariance(line);
  const double inf = std::numeric_limits<double>::infinity();
  RC_CHECK_NEAR(mahalanobis_squared(m, m, c), 0.0, 1e-12);
  RC_CHECK(mahalanobis_squared(Pt{m.x + 1.0, m.y}, m, c) == inf);
  RC_CHECK(!inside_ellipse(Pt{m.x + 1.0, m.y}, m, c, kInside95));
}
