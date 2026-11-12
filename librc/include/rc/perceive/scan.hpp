// rc/perceive/scan.hpp
//
// A lidar scan to points, from lesson 24-01.
//
// A LidarScan is a fan of ranges from an angle in steps (and the range limits and the time the sweep took). scan_to_points gives the points in the sensor's frame, dropping every beam that is infinite, not a number, nearer than the minimum or farther than the maximum; transform_points moves points into another frame with a transform of lesson 22-02, so that a sensor on a mount is seen from the vehicle. point_spacing is the distance between the points of two neighbouring beams at a range, min_detectable_width is the narrowest thing that gets a given number of beams at a range, and max_range_for_width is the farthest range at which a thing of a width still gets that many.
//
// A scan of a room of 360 beams gave 360 points, the wall at 5 m at x = 5.000; seen from a sensor tilted 5 degrees down the same wall points had heights from -0.19 to 0.69 m; at 10 m, with one degree between beams, the points were 0.175 m apart, a post of 0.3 m was hit at most 17.2 m away, and with 10 percent of the beams lost the number of points was 325.

#ifndef RC_RC_PERCEIVE_SCAN_HPP
#define RC_RC_PERCEIVE_SCAN_HPP

#include <cmath>
#include <cstddef>
#include <limits>
#include <rc/field/frames.hpp>
#include <vector>

namespace rc {
namespace perceive {

// A fan of ranges: the angle of the first beam, the step between beams (radians), the limits of what the sensor reports, and the range of each beam. A beam that saw nothing is infinity (lesson 22-06).
struct LidarScan {
  double angle_min = 0.0;
  double angle_inc = 0.0;
  double min_range = 0.0;
  double max_range = 0.0;
  std::vector<double> ranges;
};

// The points a scan sees, in the sensor's frame (x ahead, y to the left, z up, all in the plane of the scan): every beam with a range that is a number, at least the minimum and at most the maximum.
inline std::vector<rc::field::Point3> scan_to_points(const LidarScan& scan) {
  std::vector<rc::field::Point3> points;
  for (std::size_t i = 0; i < scan.ranges.size(); ++i) {
    const double r = scan.ranges[i];
    if (!(r == r) || r < scan.min_range || r > scan.max_range) continue;
    const double a = scan.angle_min + scan.angle_inc * static_cast<double>(i);
    rc::field::Point3 p;
    p.x = r * std::cos(a);
    p.y = r * std::sin(a);
    p.z = 0.0;
    points.push_back(p);
  }
  return points;
}

inline std::vector<rc::field::Point3> transform_points(const std::vector<rc::field::Point3>& points, const rc::field::Tf3& to) {
  std::vector<rc::field::Point3> out;
  out.reserve(points.size());
  for (std::size_t i = 0; i < points.size(); ++i) out.push_back(rc::field::apply(to, points[i]));
  return out;
}

inline double valid_fraction(const LidarScan& scan) {
  if (scan.ranges.empty()) return 0.0;
  return static_cast<double>(scan_to_points(scan).size()) / static_cast<double>(scan.ranges.size());
}

// The distance between the points of two neighbouring beams at a range, for a small step.
inline double point_spacing(double range, double angle_inc) { return range * angle_inc; }

// The narrowest thing that gets `hits` beams at a range: it must be wider than (hits - 1) gaps.
inline double min_detectable_width(double range, double angle_inc, int hits) { return (static_cast<double>(hits) - 1.0) * point_spacing(range, angle_inc); }

// The farthest range at which a thing of a width still gets `hits` beams.
inline double max_range_for_width(double width, double angle_inc, int hits) { return width / ((static_cast<double>(hits) - 1.0) * angle_inc); }


}  // namespace perceive
}  // namespace rc

#endif
