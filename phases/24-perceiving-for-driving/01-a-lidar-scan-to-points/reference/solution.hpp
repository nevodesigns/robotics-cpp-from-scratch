#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cmath>
#include <cstddef>
#include <limits>
#include <rc/field/frames.hpp>
#include <vector>

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


#endif
