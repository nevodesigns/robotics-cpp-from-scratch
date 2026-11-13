// rc/drive/protective_field.hpp
//
// A field that moves with the vehicle, from lesson 27-03.
//
// field_length is the distance the vehicle needs to stop at a speed: the distance it goes in the latency, plus the braking distance, plus a margin. in_swept_field says whether a point in the vehicle's frame (x ahead, y to the left) lies in the corridor the vehicle sweeps over a length on an arc of a curvature: for a straight, a rectangle; for a turn, a ring of the width of the vehicle round the arc, as far along it as the length. evaluate_field tests the points of a scan against a stop field of the field length and a warning field of twice that, and gives clear, warn or stop. protective_speed turns that into a speed: unchanged, no more than a creep speed, or zero. select_field_set picks, from the few sets a device has, the smallest that was designed for at least the speed, and says when no set is big enough.
//
// A vehicle at 1.5 m/s that stopped when something entered its field stopped 0.20 m short of it, which is the margin; one whose field had been worked out with no latency, when the latency was 0.3 s, stopped 0.25 m past where the thing was. An obstacle on the arc of a turn of radius 2 m at 40 degrees round, which the straight field missed, was found by the swept field. At 1.1 m/s the set designed for 1.5 m/s was 1.46 times longer than the speed needed, and at 2.1 m/s no set of a device built for 2.0 m/s was big enough.

#ifndef RC_RC_DRIVE_PROTECTIVE_FIELD_HPP
#define RC_RC_DRIVE_PROTECTIVE_FIELD_HPP

#include <cmath>
#include <cstddef>
#include <vector>

namespace rc {
namespace drive {

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
  if (std::fabs(curvature) < 1e-6) return px >= 0.0 && px <= length && std::fabs(py) <= half_width;
  const double R = 1.0 / std::fabs(curvature);
  const double sign = curvature > 0.0 ? 1.0 : -1.0;
  const double cy = sign * R;
  if (std::fabs(std::hypot(px, py - cy) - R) > half_width) return false;
  const double phi = std::atan2(px, -sign * (py - cy));
  return phi >= 0.0 && phi * R <= length;
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


}  // namespace drive
}  // namespace rc

#endif
