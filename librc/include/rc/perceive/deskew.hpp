// rc/perceive/deskew.hpp
//
// A scan taken while moving, from lesson 24-07.
//
// A sweep takes time, and beam k of N is fired at (k + 1/2) / N of it. interpolate_pose gives the pose of the sensor at a fraction of the way from its pose at the start of the sweep to its pose at the end (linearly in position, and by the shortest turn in heading), which is right for a constant velocity. deskew takes the scan and the two poses and puts each valid point into the frame of the start pose by way of the pose its beam was fired from. skew_distance and skew_angle are what the sweep alone moves the sensor by at a speed and a turn rate, and far_displacement is how far a point at a range moves for a turn.
//
// A sweep of 0.1 s on a vehicle turning at 1 rad/s turned the sensor 5.7 degrees across it; a wall 5 m ahead read as 0.119 m from straight in the point cloud and 0.0000 m after deskewing; the vehicle driving at 2 m/s at the same wall read 0.101 m and 0.0000 m; at 5 m/s a sweep skewed 0.50 m, and a point 20 m away moved 2.00 m for a turn of 1 rad/s.

#ifndef RC_RC_PERCEIVE_DESKEW_HPP
#define RC_RC_PERCEIVE_DESKEW_HPP

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <rc/field/frames.hpp>
#include <rc/perceive/scan.hpp>
#include <vector>

namespace rc {
namespace perceive {

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


}  // namespace perceive
}  // namespace rc

#endif
