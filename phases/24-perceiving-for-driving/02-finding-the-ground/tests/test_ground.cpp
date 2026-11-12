#include <rc/test/rc_test.hpp>

#include <cmath>
#include <iostream>

#include <rc/core/random.hpp>

#include "solution.hpp"
#include <iomanip>
#include <iostream>

static void rec(const char* key, double value, int digits = 3) {
  std::cout << "@" << key << " " << std::fixed << std::setprecision(digits) << value << std::endl;
  std::cout.unsetf(std::ios::floatfield);
}
[[maybe_unused]] static void recs(const char* key, const std::string& value) { std::cout << "@" << key << " " << value << std::endl; }


namespace {

const double kPi = 3.14159265358979323846;

// The ground tilted 5 degrees about y (rising towards +x), 2000 points over x in [1, 10] and y in [-5, 5], with noise of 1 cm; and three boxes up to 1 m high standing on it, towards the far end.
std::vector<rc::field::Point3> scene(unsigned int seed, bool boxes) {
  rc::core::Rng rng(seed);
  std::vector<rc::field::Point3> pts;
  const double slope = std::tan(5.0 * kPi / 180.0);
  for (int i = 0; i < 2000; ++i) {
    rc::field::Point3 p;
    p.x = 1.0 + 9.0 * rng.uniform();
    p.y = -5.0 + 10.0 * rng.uniform();
    p.z = slope * p.x + 0.01 * rng.gaussian();
    pts.push_back(p);
  }
  if (boxes) {
    const double bx[3] = {6.5, 8.0, 9.2};
    const double by[3] = {-2.0, 1.0, -1.0};
    for (int b = 0; b < 3; ++b) {
      for (int i = 0; i < 150; ++i) {
        rc::field::Point3 p;
        p.x = bx[b] + 0.5 * (rng.uniform() - 0.5);
        p.y = by[b] + 0.5 * (rng.uniform() - 0.5);
        p.z = slope * p.x + 1.0 * rng.uniform();
        pts.push_back(p);
      }
    }
  }
  return pts;
}

}  // namespace

RC_TEST("three points make a plane, the distance to it is signed, and the tilt is read from the normal") {
  rc::field::Point3 a{0, 0, 0}, b{1, 0, 0}, c{0, 1, 0};
  const Plane flat = plane_from_points(a, b, c);
  RC_CHECK_NEAR(flat.nz, 1.0, 1e-12);
  RC_CHECK_NEAR(tilt_degrees(flat), 0.0, 1e-9);
  rc::field::Point3 above{3, 4, 0.7};
  RC_CHECK_NEAR(plane_distance(flat, above), 0.7, 1e-12);
  RC_CHECK_NEAR(plane_distance(flat, rc::field::Point3{3, 4, -0.2}), -0.2, 1e-12);
  const Plane tilted = plane_from_points(rc::field::Point3{0, 0, 0}, rc::field::Point3{1, 0, std::tan(10.0 * kPi / 180.0)}, rc::field::Point3{0, 1, 0});
  RC_CHECK_NEAR(tilt_degrees(tilted), 10.0, 1e-9);
  RC_CHECK(plane_from_points(a, b, rc::field::Point3{2, 0, 0}).nz == 0.0);
}

RC_TEST("what stands on the ground pulls a least squares plane off it and does not move a plane of random samples") {
  const std::vector<rc::field::Point3> clean = scene(1, false);
  const std::vector<rc::field::Point3> pts = scene(1, true);
  const double ls_clean = tilt_degrees(fit_plane_least_squares(clean));
  const double ls = tilt_degrees(fit_plane_least_squares(pts));
  const RansacResult r = fit_plane_ransac(pts, 200, 0.03, 30.0, 7);
  const double rt = tilt_degrees(r.plane);
  rec("ls_tilt", ls, 2);
  rec("ransac_tilt", rt, 2);
  rec("inlier_pct", 100.0 * r.inliers / static_cast<double>(pts.size()), 0);
  std::cout << "  true tilt 5.00; least squares on the ground alone " << ls_clean << ", with the boxes " << ls << "; random samples " << rt << " with " << r.inliers << " of " << pts.size() << " points\n";
  RC_CHECK_NEAR(ls_clean, 5.0, 0.1);
  RC_CHECK(std::fabs(ls - 5.0) > 0.4);
  RC_CHECK_NEAR(rt, 5.0, 0.3);
  RC_CHECK(r.inliers > 1800);
  // The same seed draws the same points.
  RC_CHECK(fit_plane_ransac(pts, 200, 0.03, 30.0, 7).inliers == r.inliers);
}

RC_TEST("the ground and what stands on it are split by height above the plane") {
  const std::vector<rc::field::Point3> pts = scene(2, true);
  const RansacResult r = fit_plane_ransac(pts, 200, 0.03, 30.0, 3);
  const GroundSplit s = split_ground(pts, r.plane, 0.10);
  rec("n_obstacles", static_cast<double>(s.above.size()), 0);
  std::cout << "  " << s.ground.size() << " ground points and " << s.above.size() << " above\n";
  RC_CHECK(s.ground.size() > 1900);
  RC_CHECK(s.above.size() > 350 && s.above.size() < 460);
  for (std::size_t i = 0; i < s.above.size(); ++i) RC_CHECK(plane_distance(r.plane, s.above[i]) > 0.1);
}

RC_TEST("how many draws are enough depends on the share of points on the plane") {
  const int n85 = ransac_samples(0.85, 0.99);
  rec("samples", n85, 0);
  RC_CHECK(n85 <= 10);
  RC_CHECK(ransac_samples(0.5, 0.99) > 30);
  RC_CHECK(ransac_samples(0.3, 0.99) > 150);
  RC_CHECK(ransac_samples(1.0, 0.99) == 1);
  RC_CHECK(ransac_samples(0.0, 0.99) > 100000);
}

RC_TEST("a wall with more points than the ground is the biggest plane, and is the ground only if nothing says the ground is level") {
  rc::core::Rng rng(11);
  std::vector<rc::field::Point3> pts;
  for (int i = 0; i < 600; ++i) pts.push_back(rc::field::Point3{1.0 + 9.0 * rng.uniform(), -5.0 + 10.0 * rng.uniform(), 0.01 * rng.gaussian()});
  // A wall at x = 6, 1200 points, 0 to 3 m high.
  for (int i = 0; i < 1200; ++i) pts.push_back(rc::field::Point3{6.0 + 0.01 * rng.gaussian(), -5.0 + 10.0 * rng.uniform(), 3.0 * rng.uniform()});
  const RansacResult free = fit_plane_ransac(pts, 300, 0.03, 90.0, 5);
  const RansacResult level = fit_plane_ransac(pts, 300, 0.03, 30.0, 5);
  rec("wall_tilt", tilt_degrees(free.plane), 0);
  rec("ground_tilt", tilt_degrees(level.plane), 1);
  RC_CHECK(tilt_degrees(free.plane) > 80.0);
  RC_CHECK(tilt_degrees(level.plane) < 2.0);
  RC_CHECK(level.inliers > 500 && level.inliers < free.inliers);
}
