#include <rc/test/rc_test.hpp>

#include <cmath>
#include <iostream>

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
const double kH = 0.25;
const double kPitch = 5.0 * kPi / 180.0;

// The ranges a lidar tipped down sees of flat ground, with a hole of depth `depth` between x = hole_from and hole_to (world), the lidar at x = s.
std::vector<double> scan_of_ground(double s, double hole_from, double hole_to, double depth, const std::vector<double>& az) {
  std::vector<double> ranges;
  for (std::size_t i = 0; i < az.size(); ++i) {
    const double down = std::cos(az[i]) * std::sin(kPitch);
    if (down <= 1e-9) {
      ranges.push_back(std::numeric_limits<double>::infinity());
      continue;
    }
    const double x = s + ground_x(kH, kPitch, az[i]);
    if (x >= hole_from && x < hole_to) ranges.push_back((kH + depth) / down);  // down to the bottom
    else ranges.push_back(kH / down);
  }
  return ranges;
}

std::vector<double> azimuths() {
  std::vector<double> az;
  for (int i = -60; i <= 60; i += 2) az.push_back(i * kPi / 180.0);
  return az;
}

// How many scans, taken every `step` metres from the start offset, see the hole.
int scans_that_see(double step, double offset, double hole_from, double hole_to) {
  const std::vector<double> az = azimuths();
  int seen = 0;
  for (int k = 0; k < 40; ++k) {
    const double s = offset + k * step;
    const std::vector<double> r = scan_of_ground(s, hole_from, hole_to, 0.3, az);
    if (nearest_hole_x(az, r, kH, kPitch, 0.1) < 1e8) ++seen;
  }
  return seen;
}

}  // namespace

RC_TEST("the ground is seen along a straight line ahead, wherever the beam points, and not at all by beams that point level or away") {
  const double front = expected_ground_range(kH, kPitch, 0.0);
  const double side = expected_ground_range(kH, kPitch, 30.0 * kPi / 180.0);
  rec("front_range", front, 3);
  rec("side_range", side, 3);
  RC_CHECK_NEAR(front, kH / std::sin(kPitch), 1e-9);
  RC_CHECK_NEAR(side, kH / (std::cos(30.0 * kPi / 180.0) * std::sin(kPitch)), 1e-9);
  const double ahead = ground_x(kH, kPitch, 0.0);
  rec("ahead", ahead, 2);
  RC_CHECK_NEAR(ground_x(kH, kPitch, 30.0 * kPi / 180.0), ahead, 1e-9);
  RC_CHECK_NEAR(ground_x(kH, kPitch, -45.0 * kPi / 180.0), ahead, 1e-9);
  RC_CHECK_NEAR(ahead, kH / std::tan(kPitch), 1e-9);
  RC_CHECK(!std::isfinite(expected_ground_range(kH, kPitch, kPi / 2.0)));
  RC_CHECK(!std::isfinite(expected_ground_range(kH, kPitch, kPi)));
  RC_CHECK(!std::isfinite(expected_ground_range(kH, 0.0, 0.0)));
  rec("ahead_10", ground_x(kH, 10.0 * kPi / 180.0, 0.0), 2);
  RC_CHECK_NEAR(ground_x(kH, 10.0 * kPi / 180.0, 0.0), kH / std::tan(10.0 * kPi / 180.0), 1e-9);
}

RC_TEST("a beam is ground, a hole or raised by its range against the range of flat ground") {
  const double e = expected_ground_range(kH, kPitch, 0.0);
  RC_CHECK(classify_beam(e, e, 0.1) == Beam::ground);
  RC_CHECK(classify_beam(e + 0.05, e, 0.1) == Beam::ground);
  RC_CHECK(classify_beam(e + 0.5, e, 0.1) == Beam::hole);
  RC_CHECK(classify_beam(std::numeric_limits<double>::infinity(), e, 0.1) == Beam::hole);
  RC_CHECK(classify_beam(e - 0.5, e, 0.1) == Beam::raised);
  RC_CHECK(classify_beam(3.0, std::numeric_limits<double>::infinity(), 0.1) == Beam::no_ground_expected);
}

RC_TEST("a hole of 0.4 m is seen by the scans that fall on it, and the first one sees it as far ahead as the ground line was") {
  const std::vector<double> az = azimuths();
  const std::vector<double> clear = scan_of_ground(0.0, 100.0, 101.0, 0.3, az);
  RC_CHECK(nearest_hole_x(az, clear, kH, kPitch, 0.1) > 1e8);
  const double ahead = ground_x(kH, kPitch, 0.0);
  // The vehicle at x = 1.0: the ground line is at 1.0 + 2.858 = 3.858, inside a hole from 3.8 to 4.2.
  const std::vector<double> in_hole = scan_of_ground(1.0, 3.8, 4.2, 0.3, az);
  RC_CHECK_NEAR(nearest_hole_x(az, in_hole, kH, kPitch, 0.1), ahead, 1e-6);
  const int slow = scans_that_see(0.1, 0.0, 3.8, 4.2);  // 1 m/s, a scan every 0.1 s
  rec("seen_slow", slow, 0);
  RC_CHECK(slow >= 3 && slow <= 5);
}

RC_TEST("a hole narrower than the distance driven between scans can fall between them") {
  RC_CHECK_NEAR(max_undetected_width(5.0, 10.0), 0.5, 1e-12);
  RC_CHECK_NEAR(max_undetected_width(1.0, 10.0), 0.1, 1e-12);
  // The ground line is at s + 2.858. A hole of 0.3 m from 3.9: scans at s = 0, 0.5, 1.0 ... put it at 2.858, 3.358, 3.858 (miss), 4.358: it is missed.
  const int fast_missed = scans_that_see(0.5, 0.0, 3.9, 4.2);
  const int fast_hit = scans_that_see(0.5, 0.3, 3.9, 4.2);
  rec("seen_fast_missed", fast_missed, 0);
  rec("seen_fast_hit", fast_hit, 0);
  RC_CHECK(fast_missed == 0);
  RC_CHECK(fast_hit >= 1);
}

RC_TEST("the speed to stop before ground first seen at a distance falls with the latency and rises with the braking") {
  const double ahead = ground_x(kH, kPitch, 0.0);
  const double v = max_safe_speed(ahead, 0.3, 1.5, 0.2);
  rec("vmax", v, 2);
  // At that speed the distance covered is exactly the room.
  RC_CHECK_NEAR(v * 0.2 + v * v / (2.0 * 1.5), ahead - 0.3, 1e-9);
  RC_CHECK(max_safe_speed(ahead, 0.3, 1.5, 0.0) > v);
  RC_CHECK(max_safe_speed(ahead, 0.3, 3.0, 0.2) > v);
  RC_CHECK(max_safe_speed(0.2, 0.3, 1.5, 0.2) == 0.0);
  // Tipping the sensor down further brings the ground nearer and the vehicle must go slower.
  RC_CHECK(max_safe_speed(ground_x(kH, 10.0 * kPi / 180.0, 0.0), 0.3, 1.5, 0.2) < v);
}
