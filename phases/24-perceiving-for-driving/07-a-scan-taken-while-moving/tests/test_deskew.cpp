#include <rc/test/rc_test.hpp>

#include <cmath>
#include <iostream>

#include <rc/field/sensors.hpp>

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
const double kSweep = 0.1;
const int kBeams = 360;

// Where the sensor is at a time during the sweep: it drives at v and turns at w from the origin facing +x.
rc::field::Tf2 truth(double v, double w, double t) {
  rc::field::Tf2 p;
  if (std::fabs(w) < 1e-9) {
    p.x = v * t;
    p.theta = 0.0;
  } else {
    p.x = v / w * std::sin(w * t);
    p.y = v / w * (1.0 - std::cos(w * t));
    p.theta = w * t;
  }
  return p;
}

// The scan that a sensor moving like that makes of a straight wall at x = 5: each beam is cast from where the sensor is when it is fired.
LidarScan sweep(double v, double w) {
  rc::field::World2 world;
  world.walls.push_back(rc::field::Segment{5.0, -30.0, 5.0, 30.0});
  LidarScan s;
  s.angle_min = -kPi + 0.5 * 2.0 * kPi / kBeams;
  s.angle_inc = 2.0 * kPi / kBeams;
  s.min_range = 0.15;
  s.max_range = 40.0;
  for (int k = 0; k < kBeams; ++k) {
    const rc::field::Tf2 p = truth(v, w, (k + 0.5) / kBeams * kSweep);
    const double beam = s.angle_min + s.angle_inc * k;
    s.ranges.push_back(rc::field::cast_ray(world, p.x, p.y, p.theta + beam, 40.0));
  }
  return s;
}

// The root mean square distance of the points that look at the wall (within 40 degrees of ahead) from the line x = 5, which is where the wall is in the frame of the start pose.
double rms_from_wall(const std::vector<rc::field::Point3>& pts) {
  double sum = 0.0;
  int n = 0;
  for (std::size_t i = 0; i < pts.size(); ++i) {
    if (pts[i].x > 3.0 && std::fabs(std::atan2(pts[i].y, pts[i].x)) < 40.0 * kPi / 180.0) {
      sum += (pts[i].x - 5.0) * (pts[i].x - 5.0);
      ++n;
    }
  }
  return n > 0 ? std::sqrt(sum / n) : -1.0;
}

}  // namespace

RC_TEST("a pose halfway is halfway in position and by the short way in heading") {
  rc::field::Tf2 a, b;
  b.x = 2.0;
  b.y = -1.0;
  b.theta = 0.6;
  const rc::field::Tf2 m = interpolate_pose(a, b, 0.5);
  RC_CHECK_NEAR(m.x, 1.0, 1e-12);
  RC_CHECK_NEAR(m.y, -0.5, 1e-12);
  RC_CHECK_NEAR(m.theta, 0.3, 1e-12);
  rc::field::Tf2 c, d;
  c.theta = 3.0;
  d.theta = -3.0;
  const rc::field::Tf2 across = interpolate_pose(c, d, 0.5);
  RC_CHECK(std::fabs(std::remainder(across.theta - kPi, 2.0 * kPi)) < 0.2);
  RC_CHECK_NEAR(interpolate_pose(a, b, 0.0).x, 0.0, 1e-12);
  RC_CHECK_NEAR(interpolate_pose(a, b, 1.0).theta, 0.6, 1e-12);
}

RC_TEST("a turning sensor makes a wall look rotated, and deskewing straightens it") {
  const LidarScan s = sweep(0.0, 1.0);
  const rc::field::Tf2 start;
  const rc::field::Tf2 end = truth(0.0, 1.0, kSweep);
  const double before = rms_from_wall(scan_to_points(s));
  const double after = rms_from_wall(deskew(s, start, end));
  rec("rms_rot_before", before, 3);
  rec("rms_rot_after", after, 4);
  std::cout << "  the wall straight ahead: " << before << " m rms from straight as it was, " << after << " m after deskewing\n";
  RC_CHECK(before > 0.05);
  RC_CHECK(after < 0.01);
  rec("turned_deg", skew_angle(1.0, kSweep) * 180.0 / kPi, 1);
}

RC_TEST("a vehicle driving at the wall makes it look slanted, and deskewing straightens that too") {
  const LidarScan s = sweep(2.0, 0.0);
  const rc::field::Tf2 start;
  const rc::field::Tf2 end = truth(2.0, 0.0, kSweep);
  const double before = rms_from_wall(scan_to_points(s));
  const double after = rms_from_wall(deskew(s, start, end));
  rec("rms_trans_before", before, 3);
  rec("rms_trans_after", after, 4);
  RC_CHECK(before > 0.02);
  RC_CHECK(after < 0.01);
}

RC_TEST("a sensor that does not move is not changed by deskewing") {
  const LidarScan s = sweep(0.0, 0.0);
  const rc::field::Tf2 start;
  const std::vector<rc::field::Point3> a = scan_to_points(s);
  const std::vector<rc::field::Point3> b = deskew(s, start, start);
  RC_CHECK(a.size() == b.size());
  for (std::size_t i = 0; i < a.size(); ++i) {
    RC_CHECK_NEAR(a[i].x, b[i].x, 1e-12);
    RC_CHECK_NEAR(a[i].y, b[i].y, 1e-12);
  }
}

RC_TEST("how far a sweep skews the cloud grows with speed, and a turn moves the far points most") {
  rec("skew", skew_distance(5.0, kSweep), 2);
  rec("far", far_displacement(20.0, 1.0, kSweep), 2);
  RC_CHECK_NEAR(skew_distance(5.0, kSweep), 0.5, 1e-12);
  RC_CHECK_NEAR(far_displacement(20.0, 1.0, kSweep), 2.0, 1e-12);
  RC_CHECK(far_displacement(20.0, 1.0, kSweep) > far_displacement(5.0, 1.0, kSweep));
}
