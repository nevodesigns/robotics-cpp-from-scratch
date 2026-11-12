// rc/perceive/drops.hpp
//
// Holes and drops, from lesson 24-06.
//
// A planar lidar tipped down by an angle (pitch) at a height h has its beams hit flat ground along a straight line across the front of the vehicle, h / tan(pitch) ahead. expected_ground_range gives the range of the beam in a direction (azimuth) for that ground, and infinity for a direction that does not meet it. classify_beam compares what a beam returned with that: near the expected range is ground; longer or no return is a hole (the ground is lower than it should be, or not there); shorter is something raised on it. nearest_hole_x gives how far ahead the first hole is. max_undetected_width is the distance the vehicle drives between two scans, the widest hole that can fall between them; max_safe_speed is the speed from which the vehicle can stop, with its latency and its braking, before ground that it first sees at a distance.
//
// A lidar 0.25 m up and tipped down 5 degrees saw flat ground 2.86 m ahead in every direction (a beam 30 degrees to the side had a range of 3.312 m and the one ahead of 2.868 m); tipped down 10 degrees the ground was 1.42 m ahead; a hole of 0.4 m was seen in 4 scans at 1 m/s and a hole of 0.3 m missed at 5 m/s with a scan every 0.1 s; and at a braking of 1.5 m/s2 with 0.2 s of latency the fastest the vehicle could go and still stop before ground first seen 2.86 m away was 2.49 m/s.

#ifndef RC_RC_PERCEIVE_DROPS_HPP
#define RC_RC_PERCEIVE_DROPS_HPP

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>
#include <vector>

namespace rc {
namespace perceive {

// The range of the beam of a planar lidar, at a height above the ground and tipped down by `pitch` (radians), in the direction `azimuth` (radians from straight ahead, in the plane of the scan), to flat level ground. Infinity if the beam goes level or upwards, or sideways.
inline double expected_ground_range(double height, double pitch, double azimuth) {
  const double down = std::cos(azimuth) * std::sin(pitch);
  if (down <= 1e-9) return std::numeric_limits<double>::infinity();
  return height / down;
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


}  // namespace perceive
}  // namespace rc

#endif
