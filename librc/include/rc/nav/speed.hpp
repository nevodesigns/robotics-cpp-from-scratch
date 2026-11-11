// rc/nav/speed.hpp
//
// Speed for a path, from lesson 16-11, graduated.
//
// Curvature from three points (2 x area over the product of the sides), a speed limit from sideways acceleration (v squared x k at most a_lat, so v at most sqrt(a_lat / k)),
// and a profile that never gains more than the wheels can accelerate or loses more than they can brake, by a forward pass and a backward pass from the stop.
//
// A route of straight, quarter circle, straight at a steady 2 m/s felt 4.00 m/s^2 sideways on a radius of 1 m; with limits 1.00. The limits alone asked for a gain and a loss of 19.81 m/s^2 in one step;
// the profile stayed within 1.00 gain and 1.50 loss. Time to drive 20 m of straight and the corner: 12.45 s at a corner speed of 2.00, 12.90 at 1.41, 13.60 at 1.00 and 14.49 at 0.71 m/s.

#ifndef RC_NAV_SPEED_HPP
#define RC_NAV_SPEED_HPP

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <vector>
#include <rc/math/vector.hpp>

namespace rc {
namespace nav {

using rc::math::Vec2;

// The curvature, one over the radius, of the circle through three points. Zero if two of them coincide or all three are in a line.
// It is 2 times the area of the triangle they make over the product of its three sides.
inline double curvature_at(Vec2 a, Vec2 b, Vec2 c) {
  const double ab = std::hypot(b.x - a.x, b.y - a.y);
  const double bc = std::hypot(c.x - b.x, c.y - b.y);
  const double ca = std::hypot(a.x - c.x, a.y - c.y);
  if (ab == 0.0 || bc == 0.0 || ca == 0.0) return 0.0;
  const double cross = (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
  return 2.0 * std::fabs(cross) / (ab * bc * ca);
}

// The fastest each point of a path can be taken: no faster than `max_speed`, and slow enough that the sideways acceleration,
// speed squared times curvature, stays within `max_lateral`. The two ends have no neighbour on one side and get `max_speed`.
inline std::vector<double> speed_limits(const std::vector<Vec2>& path, double max_speed, double max_lateral) {
  std::vector<double> limits(path.size(), max_speed);
  for (std::size_t i = 1; i + 1 < path.size(); ++i) {
    const double k = curvature_at(path[i - 1], path[i], path[i + 1]);
    if (k > 0.0 && max_lateral > 0.0) limits[i] = std::min(max_speed, std::sqrt(max_lateral / k));
  }
  return limits;
}

inline double distance_between(Vec2 a, Vec2 b) { return std::hypot(b.x - a.x, b.y - a.y); }

// A speed for each point that respects the limits and also the wheels: it never gains more than `max_accel` and never needs
// to lose more than `max_decel`. The first point starts at `start_speed` (or its limit if that is lower) and the last comes to rest.
// A forward pass for acceleration, then a backward pass from the stop for braking, using v squared = u squared + 2 a d.
inline std::vector<double> speed_profile(const std::vector<Vec2>& path, const std::vector<double>& limits, double max_accel,
                                         double max_decel, double start_speed) {
  std::vector<double> v = limits;
  if (path.empty() || limits.size() != path.size()) return std::vector<double>();
  v.back() = 0.0;
  v.front() = std::min(v.front(), std::max(start_speed, 0.0));
  for (std::size_t i = 1; i < v.size(); ++i) {
    const double d = distance_between(path[i - 1], path[i]);
    v[i] = std::min(v[i], std::sqrt(v[i - 1] * v[i - 1] + 2.0 * max_accel * d));
  }
  for (std::size_t i = v.size() - 1; i > 0; --i) {
    const double d = distance_between(path[i - 1], path[i]);
    v[i - 1] = std::min(v[i - 1], std::sqrt(v[i] * v[i] + 2.0 * max_decel * d));
  }
  return v;
}

// Time to drive the path with these speeds: each piece takes its length over the mean of its two end speeds.
// A piece with both ends at rest cannot be driven, and makes the answer negative.
inline double travel_time(const std::vector<Vec2>& path, const std::vector<double>& speeds) {
  if (path.size() != speeds.size() || path.size() < 2) return 0.0;
  double total = 0.0;
  for (std::size_t i = 1; i < path.size(); ++i) {
    const double mean = 0.5 * (speeds[i - 1] + speeds[i]);
    if (mean <= 0.0) return -1.0;
    total += distance_between(path[i - 1], path[i]) / mean;
  }
  return total;
}

}  // namespace nav
}  // namespace rc

#endif  // RC_NAV_SPEED_HPP
