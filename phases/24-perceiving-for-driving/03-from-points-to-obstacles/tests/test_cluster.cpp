#include <rc/test/rc_test.hpp>
#include <sstream>
#include <cstring>

#include <cmath>
#include <iostream>

#include <rc/core/random.hpp>
#include <rc/field/sensors.hpp>
#include <rc/perceive/scan.hpp>

#include "solution.hpp"
#include <iomanip>
#include <iostream>

// The numbers this lesson publishes: its page, its header and its quiz quote them, so each is held here as it was
// measured. A change that moves one by more than half a unit of its last printed digit fails the suite.
struct Pin {
  const char* key;
  double value;
};
static const Pin kPins[] = {
    {"n_right", 4.0},
    {"centroid_err", 8.881784197001252e-16},
    {"n_small", 0.0},
    {"n_merged", 3.0},
    {"n_all", 1.0},
    {"wall_fixed_small", 0.0},
    {"wall_adaptive", 1.0},
    {"posts_fixed", 1.0},
    {"posts_adaptive", 2.0},
};

static void rec(const char* key, double value, int digits = 3) {
  std::cout << "@" << key << " " << std::fixed << std::setprecision(digits) << value << std::endl;
  std::cout.unsetf(std::ios::floatfield);
  for (const Pin& pin : kPins) {
    if (std::strcmp(pin.key, key) != 0) continue;
    double allowed = 0.5;
    for (int i = 0; i < digits; ++i) allowed /= 10.0;
    if (!(std::fabs(value - pin.value) <= allowed)) {
      std::ostringstream message;
      message << "the published number " << key << " was " << pin.value << " and is now " << value;
      rc::test::record(__FILE__, __LINE__, message.str());
    }
  }
}
[[maybe_unused]] static void recs(const char* key, const std::string& value) { std::cout << "@" << key << " " << value << std::endl; }


namespace {

const double kPi = 3.14159265358979323846;

// Points on a ring of a radius round a centre.
void ring(std::vector<rc::field::Point3>& pts, double cx, double cy, double r, int n) {
  for (int i = 0; i < n; ++i) {
    const double a = 2.0 * kPi * i / n;
    pts.push_back(rc::field::Point3{cx + r * std::cos(a), cy + r * std::sin(a), 0.0});
  }
}

std::vector<rc::field::Point3> scene() {
  std::vector<rc::field::Point3> pts;
  ring(pts, 3.0, 2.0, 0.15, 20);   // a post
  ring(pts, 6.0, -1.0, 0.25, 24);  // a person
  ring(pts, 8.0, 1.0, 0.15, 20);   // a post
  ring(pts, 8.8, 1.0, 0.15, 20);   // another, 0.8 m from it (0.5 m between their faces)
  // scattered single points
  pts.push_back(rc::field::Point3{4.5, 4.0, 0.0});
  pts.push_back(rc::field::Point3{9.0, -3.0, 0.0});
  pts.push_back(rc::field::Point3{1.0, -2.0, 0.0});
  return pts;
}

// The points a lidar with one degree between beams sees of a wall at x = 11 and of two posts at x = 2, off to one side so that they do not hide the wall.
std::vector<rc::field::Point3> scanned() {
  rc::field::World2 w;
  w.walls.push_back(rc::field::Segment{11.0, -3.0, 11.0, 3.0});
  w.posts.push_back(rc::field::Circle{2.0, 1.45, 0.05});
  w.posts.push_back(rc::field::Circle{2.0, 1.80, 0.05});
  rc::field::LidarSpec spec;
  spec.beams = 360;
  spec.sigma = 0.0;
  spec.dropout = 0.0;
  rc::core::Rng rng(1);
  LidarScan s;
  s.angle_min = -kPi + 0.5 * spec.fov / spec.beams;
  s.angle_inc = spec.fov / spec.beams;
  s.min_range = spec.min_range;
  s.max_range = spec.max_range;
  s.ranges = rc::field::scan(spec, w, 0.0, 0.0, 0.0, rng);
  return scan_to_points(s);
}

std::size_t count_near(const std::vector<Cluster>& c, double x, double y, double within) {
  std::size_t n = 0;
  for (std::size_t i = 0; i < c.size(); ++i)
    if (std::hypot(c[i].cx - x, c[i].cy - y) < within) ++n;
  return n;
}

}  // namespace

RC_TEST("four objects among scattered points are found as four clusters, at the right places, and the scattered points are dropped") {
  const std::vector<rc::field::Point3> pts = scene();
  const std::vector<Cluster> c = cluster_points(pts, 0.2, 0.0, 3);
  rec("n_right", static_cast<double>(c.size()), 0);
  RC_CHECK(c.size() == 4);
  double worst = 0.0;
  const double truth[4][2] = {{3.0, 2.0}, {6.0, -1.0}, {8.0, 1.0}, {8.8, 1.0}};
  for (int k = 0; k < 4; ++k) {
    RC_CHECK(count_near(c, truth[k][0], truth[k][1], 0.05) == 1);
    for (std::size_t i = 0; i < c.size(); ++i)
      if (std::hypot(c[i].cx - truth[k][0], c[i].cy - truth[k][1]) < 0.05) worst = std::max(worst, std::hypot(c[i].cx - truth[k][0], c[i].cy - truth[k][1]));
  }
  rec("centroid_err", worst, 4);
  for (std::size_t i = 0; i < c.size(); ++i) RC_CHECK(c[i].members.size() >= 3);
  // The person is half a metre across.
  for (std::size_t i = 0; i < c.size(); ++i)
    if (std::fabs(c[i].cx - 6.0) < 0.1) RC_CHECK_NEAR(c[i].size_x, 0.5, 0.02);
}

RC_TEST("too small a radius sees no objects, too large joins neighbours, and the right one lies between") {
  const std::vector<rc::field::Point3> pts = scene();
  const double small = static_cast<double>(cluster_points(pts, 0.02, 0.0, 3).size());
  const double merged = static_cast<double>(cluster_points(pts, 0.6, 0.0, 3).size());
  const double all = static_cast<double>(cluster_points(pts, 5.0, 0.0, 3).size());
  rec("n_small", small, 0);
  rec("n_merged", merged, 0);
  rec("n_all", all, 0);
  RC_CHECK(small == 0.0);
  RC_CHECK(merged == 3.0);
  RC_CHECK(all == 1.0);
  // A floor of one point keeps the scattered points as objects of their own.
  RC_CHECK(cluster_points(pts, 0.2, 0.0, 1).size() == 7);
}

RC_TEST("a wall far away is broken into points by a fixed radius, and held together by a radius that grows with range") {
  const std::vector<rc::field::Point3> pts = scanned();
  const std::vector<Cluster> fixed = cluster_points(pts, 0.15, 0.0, 3);
  // The beams are 0.0175 rad apart: at 11 m the points are 0.19 m apart, wider than 0.15.
  std::size_t far_fixed = 0;
  for (std::size_t i = 0; i < fixed.size(); ++i)
    if (fixed[i].cx > 10.0) ++far_fixed;
  const std::vector<Cluster> grown = cluster_points(pts, 0.05, 1.5 * 0.0174533, 3);
  std::size_t far_grown = 0;
  for (std::size_t i = 0; i < grown.size(); ++i)
    if (grown[i].cx > 10.0) ++far_grown;
  rec("wall_fixed_small", static_cast<double>(far_fixed), 0);
  rec("wall_adaptive", static_cast<double>(far_grown), 0);
  RC_CHECK(far_fixed == 0);
  RC_CHECK(far_grown == 1);
}

RC_TEST("two posts close together are one object to a radius that is too big for the near range, and two to one that grows with it") {
  const std::vector<rc::field::Point3> pts = scanned();
  // Seen from 2 m, the two posts are 0.35 m apart, with a gap of about 0.25 m between their faces; their points are 3.5 cm apart.
  std::size_t near_fixed = 0, near_grown = 0;
  const std::vector<Cluster> fixed = cluster_points(pts, 0.4, 0.0, 1);
  for (std::size_t i = 0; i < fixed.size(); ++i)
    if (fixed[i].cx < 3.0) ++near_fixed;
  const std::vector<Cluster> grown = cluster_points(pts, 0.05, 1.5 * 0.0174533, 1);
  for (std::size_t i = 0; i < grown.size(); ++i)
    if (grown[i].cx < 3.0) ++near_grown;
  rec("posts_fixed", static_cast<double>(near_fixed), 0);
  rec("posts_adaptive", static_cast<double>(near_grown), 0);
  RC_CHECK(near_fixed == 1);
  RC_CHECK(near_grown == 2);
}
