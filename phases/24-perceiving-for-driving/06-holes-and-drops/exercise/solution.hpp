#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>
#include <vector>

// The range of the beam of a planar lidar, at a height above the ground and tipped down by `pitch` (radians), in the direction `azimuth` (radians from straight ahead, in the plane of the scan), to flat level ground. Infinity if the beam goes level or upwards, or sideways.
inline double expected_ground_range(double height, double pitch, double azimuth) {
  (void)height;  // TODO: in the sensor's frame the beam is (cos az, sin az, 0); tipped down by the pitch its downward part is cos(az) sin(pitch); it meets the ground at the height divided by that, and never if it is not downward
  (void)pitch;
  (void)azimuth;
  return 0.0;
}

// How far ahead of the sensor (along the vehicle) a beam in that direction meets the ground.
inline double ground_x(double height, double pitch, double azimuth) {
  const double t = expected_ground_range(height, pitch, azimuth);
  return std::isfinite(t) ? t * std::cos(azimuth) * std::cos(pitch) : std::numeric_limits<double>::infinity();
}

enum class Beam { no_ground_expected, ground, hole, raised };

// What a beam returned (infinity for nothing), against what flat ground would give: within `margin` of it is ground; longer, or nothing, is a hole; shorter is raised.
inline Beam classify_beam(double measured, double expected, double margin) {
  if (!std::isfinite(expected)) return Beam::no_ground_expected;
  if (!std::isfinite(measured) || measured > expected + margin) return Beam::hole;
  if (measured < expected - margin) return Beam::raised;
  return Beam::ground;
}

// The distance ahead of the first beam of a scan (azimuths in radians and the ranges returned) that is a hole, or infinity if there is none.
inline double nearest_hole_x(const std::vector<double>& azimuths, const std::vector<double>& ranges, double height, double pitch, double margin) {
  double nearest = std::numeric_limits<double>::infinity();
  for (std::size_t i = 0; i < azimuths.size() && i < ranges.size(); ++i) {
    const double expected = expected_ground_range(height, pitch, azimuths[i]);
    if (classify_beam(ranges[i], expected, margin) == Beam::hole) nearest = std::min(nearest, ground_x(height, pitch, azimuths[i]));
  }
  return nearest;
}

// The widest hole that can fall between two scans: the distance driven between them.
inline double max_undetected_width(double speed, double scan_rate) { return scan_rate > 0.0 ? speed / scan_rate : std::numeric_limits<double>::infinity(); }

// The fastest speed from which a vehicle can stop short of ground first seen `distance` ahead, keeping `margin` clear: during the latency it keeps going, and then it brakes: distance - margin = v T + v^2 / (2 a).
inline double max_safe_speed(double distance, double margin, double decel, double latency) {
  const double room = distance - margin;
  if (room <= 0.0) return 0.0;
  return decel * (-latency + std::sqrt(latency * latency + 2.0 * room / decel));
}


#endif
