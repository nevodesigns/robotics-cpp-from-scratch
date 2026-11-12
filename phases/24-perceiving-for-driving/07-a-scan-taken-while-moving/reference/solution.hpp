#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <rc/field/frames.hpp>
#include <rc/perceive/scan.hpp>
#include <vector>

using namespace rc::perceive;

// The pose at a fraction u (0 to 1) of the way from a to b: position linearly, heading by the shorter way round.
inline rc::field::Tf2 interpolate_pose(const rc::field::Tf2& a, const rc::field::Tf2& b, double u) {
  rc::field::Tf2 p;
  p.x = a.x + u * (b.x - a.x);
  p.y = a.y + u * (b.y - a.y);
  const double d = std::atan2(std::sin(b.theta - a.theta), std::cos(b.theta - a.theta));
  p.theta = a.theta + u * d;
  return p;
}

// The points of a scan, put in the frame of the pose the sweep began from, each by way of the pose its own beam was fired from. The sweep took one scan time and the sensor went from `start` to `end`; both poses are in the same world frame, and the sensor's own frame is the one of the scan.
inline std::vector<rc::field::Point3> deskew(const LidarScan& scan, const rc::field::Tf2& start, const rc::field::Tf2& end) {
  std::vector<rc::field::Point3> points;
  const std::size_t n = scan.ranges.size();
  for (std::size_t i = 0; i < n; ++i) {
    const double r = scan.ranges[i];
    if (!(r == r) || r < scan.min_range || r > scan.max_range) continue;
    const double a = scan.angle_min + scan.angle_inc * static_cast<double>(i);
    rc::field::Tf2 local;
    local.x = r * std::cos(a);
    local.y = r * std::sin(a);
    const double u = (static_cast<double>(i) + 0.5) / static_cast<double>(n);
    const rc::field::Tf2 fired_from = interpolate_pose(start, end, u);
    const rc::field::Tf2 in_start = rc::field::compose(rc::field::inverse(start), rc::field::compose(fired_from, local));
    rc::field::Point3 p;
    p.x = in_start.x;
    p.y = in_start.y;
    p.z = 0.0;
    points.push_back(p);
  }
  return points;
}

// How far a sweep moves the sensor along its way at a speed, and how far it turns it at a turn rate.
inline double skew_distance(double speed, double sweep_seconds) { return speed * sweep_seconds; }
inline double skew_angle(double turn_rate, double sweep_seconds) { return turn_rate * sweep_seconds; }

// How far a point at a range is moved by a turn of the sensor.
inline double far_displacement(double range, double turn_rate, double sweep_seconds) { return range * turn_rate * sweep_seconds; }


#endif
