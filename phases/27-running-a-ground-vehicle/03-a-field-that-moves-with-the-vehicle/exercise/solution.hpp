#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cmath>
#include <cstddef>
#include <vector>

struct FieldConfig {
  double decel = 2.0;    // m/s2 the vehicle brakes at
  double latency = 0.3;  // s from the sensor seeing a thing to the brakes taking hold
  double margin = 0.2;   // m of room left when it has stopped
  double width = 0.8;    // m, the width of the vehicle plus what it needs at its sides
  double creep = 0.3;    // m/s allowed with something in the warning field
};

struct FieldPoint {
  double x = 0.0;  // ahead of the vehicle, metres
  double y = 0.0;  // to its left
};

enum FieldResult { kClear = 0, kWarn = 1, kStop = 2 };

// The distance the vehicle needs to stop from a speed, with the margin.
inline double field_length(double speed, const FieldConfig& c) { return speed * c.latency + speed * speed / (2.0 * c.decel) + c.margin; }

// Is the point in the corridor swept for `length` metres along an arc of `curvature` (positive to the left), `half_width` each side of it?
inline bool in_swept_field(double px, double py, double length, double half_width, double curvature) {
  (void)px;  // TODO: for a straight, 0 <= x <= length and |y| <= half width; for a turn, the point must be within half the width of the circle of radius 1/curvature, and no further along that circle from the vehicle than the length
  (void)py;
  (void)length;
  (void)half_width;
  (void)curvature;
  return false;
}

// The worst of the points against a stop field of the length for this speed and a warning field of twice it.
inline FieldResult evaluate_field(const std::vector<FieldPoint>& points, double speed, double curvature, const FieldConfig& c) {
  const double stop = field_length(speed, c);
  const double half = 0.5 * c.width;
  FieldResult worst = kClear;
  for (std::size_t i = 0; i < points.size(); ++i) {
    if (in_swept_field(points[i].x, points[i].y, stop, half, curvature)) return kStop;
    if (in_swept_field(points[i].x, points[i].y, 2.0 * stop, half, curvature)) worst = kWarn;
  }
  return worst;
}

// The speed the field allows.
inline double protective_speed(FieldResult r, double speed, double creep) {
  if (r == kStop) return 0.0;
  if (r == kWarn) return speed < creep ? speed : creep;
  return speed;
}

// The smallest field set designed for at least `speed`, or -1 if none is big enough. `design` is sorted from the slowest.
inline int select_field_set(double speed, const std::vector<double>& design) {
  for (std::size_t i = 0; i < design.size(); ++i)
    if (design[i] >= speed) return static_cast<int>(i);
  return -1;
}


#endif
