#include <rc/test/rc_test.hpp>
#include <sstream>
#include <cstring>

#include <algorithm>
#include <cmath>
#include <iostream>

#include <rc/core/random.hpp>
#include <rc/field/sensors.hpp>

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
    {"n_points", 360.0},
    {"wall_x", 5.0},
    {"z_low", -0.18577871373829108},
    {"z_high", 0.6857787137382911},
    {"spacing", 0.17453292519943295},
    {"farthest", 17.188733853924695},
    {"n_noisy", 325.0},
    {"drop_pct", 9.722222222222221},
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

LidarScan scan_of(const rc::field::LidarSpec& spec, const std::vector<double>& ranges) {
  LidarScan s;
  s.angle_min = -0.5 * spec.fov + 0.5 * spec.fov / spec.beams;
  s.angle_inc = spec.fov / spec.beams;
  s.min_range = spec.min_range;
  s.max_range = spec.max_range;
  s.ranges = ranges;
  return s;
}

rc::field::World2 room() {
  rc::field::World2 w;
  w.walls.push_back(rc::field::Segment{-5, -5, 5, -5});
  w.walls.push_back(rc::field::Segment{5, -5, 5, 5});
  w.walls.push_back(rc::field::Segment{5, 5, -5, 5});
  w.walls.push_back(rc::field::Segment{-5, 5, -5, -5});
  return w;
}

}  // namespace

RC_TEST("a scan of a room becomes a point for every beam, and the points are where the wall is") {
  rc::field::LidarSpec spec;
  spec.sigma = 0.0;
  spec.dropout = 0.0;
  rc::core::Rng rng(1);
  const LidarScan scan = scan_of(spec, rc::field::scan(spec, room(), 0.0, 0.0, 0.0, rng));
  const std::vector<rc::field::Point3> pts = scan_to_points(scan);
  rec("n_points", static_cast<double>(pts.size()), 0);
  RC_CHECK(pts.size() == 360);
  RC_CHECK_NEAR(valid_fraction(scan), 1.0, 1e-12);
  // The beam nearest straight ahead hits the wall at x = 5.
  double nearest = 1e9;
  double wall_x = 0.0;
  for (std::size_t i = 0; i < pts.size(); ++i) {
    if (std::fabs(pts[i].y) < nearest && pts[i].x > 0.0) {
      nearest = std::fabs(pts[i].y);
      wall_x = pts[i].x;
    }
  }
  rec("wall_x", wall_x, 3);
  RC_CHECK_NEAR(wall_x, 5.0, 0.01);
  for (std::size_t i = 0; i < pts.size(); ++i) RC_CHECK(std::fabs(pts[i].x) < 5.01 && std::fabs(pts[i].y) < 5.01);
}

RC_TEST("beams that saw nothing, were too near or too far, or are not numbers give no points") {
  LidarScan s;
  s.angle_min = 0.0;
  s.angle_inc = 0.1;
  s.min_range = 0.15;
  s.max_range = 12.0;
  const double inf = std::numeric_limits<double>::infinity();
  s.ranges = {1.0, inf, 0.05, 20.0, std::nan(""), 3.0, 12.0, 0.15};
  const std::vector<rc::field::Point3> pts = scan_to_points(s);
  RC_CHECK(pts.size() == 4);
  RC_CHECK_NEAR(pts[0].x, 1.0, 1e-12);
  RC_CHECK_NEAR(pts[1].x, 3.0 * std::cos(0.5), 1e-12);
  RC_CHECK_NEAR(pts[1].y, 3.0 * std::sin(0.5), 1e-12);
  RC_CHECK_NEAR(valid_fraction(s), 0.5, 1e-12);
  RC_CHECK(scan_to_points(LidarScan()).empty());
}

RC_TEST("seen from a sensor tipped down, the points of a vertical wall are not at one height") {
  rc::field::LidarSpec spec;
  spec.sigma = 0.0;
  spec.dropout = 0.0;
  rc::core::Rng rng(1);
  const LidarScan scan = scan_of(spec, rc::field::scan(spec, room(), 0.0, 0.0, 0.0, rng));
  const rc::field::Tf3 mount = rc::field::from_rpy(0.0, 0.0, 0.25, 0.0, 5.0 * kPi / 180.0, 0.0);
  const std::vector<rc::field::Point3> pts = transform_points(scan_to_points(scan), mount);
  double lo = 1e9, hi = -1e9;
  for (std::size_t i = 0; i < pts.size(); ++i) {
    lo = std::min(lo, pts[i].z);
    hi = std::max(hi, pts[i].z);
  }
  rec("z_low", lo, 2);
  rec("z_high", hi, 2);
  // A beam straight ahead (x about 5) is lowered by 5 sin(5 degrees); one straight behind is raised by as much.
  RC_CHECK(lo < 0.25 - 0.4);
  RC_CHECK(hi > 0.25 + 0.4);
  RC_CHECK(hi - lo > 0.8);
}

RC_TEST("the points are further apart with range, and a thing is seen only if two beams hit it") {
  const double inc = 1.0 * kPi / 180.0;
  rec("spacing", point_spacing(10.0, inc), 3);
  RC_CHECK_NEAR(point_spacing(10.0, inc), 0.1745, 1e-3);
  RC_CHECK_NEAR(min_detectable_width(10.0, inc, 2), 0.1745, 1e-3);
  RC_CHECK_NEAR(min_detectable_width(10.0, inc, 3), 0.349, 2e-3);
  const double far = max_range_for_width(0.3, inc, 2);
  rec("farthest", far, 1);
  RC_CHECK_NEAR(far, 0.3 / inc, 1e-9);
  RC_CHECK(min_detectable_width(far, inc, 2) <= 0.3 + 1e-9);
  RC_CHECK(min_detectable_width(far * 1.05, inc, 2) > 0.3);
}

RC_TEST("lost beams are lost points") {
  rc::field::LidarSpec spec;
  spec.sigma = 0.0;
  spec.dropout = 0.1;
  rc::core::Rng rng(5);
  const LidarScan scan = scan_of(spec, rc::field::scan(spec, room(), 0.0, 0.0, 0.0, rng));
  const double n = static_cast<double>(scan_to_points(scan).size());
  rec("n_noisy", n, 0);
  rec("drop_pct", 100.0 * (1.0 - n / 360.0), 0);
  RC_CHECK(n < 360.0);
  RC_CHECK_NEAR(n / 360.0, 0.9, 0.06);
}
