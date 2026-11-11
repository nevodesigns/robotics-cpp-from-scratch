#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <vector>

#include <rc/math/vector.hpp>

using rc::math::Vec2;

// The curvature, one over the radius, of the circle through three points. Zero if two of them coincide or all three are in a line.
// It is 2 times the area of the triangle they make over the product of its three sides.
inline double curvature_at(Vec2 a, Vec2 b, Vec2 c) {
  // TODO: the sides ab, bc and ca (std::hypot); if any is zero return 0.
  // cross = (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x).
  // curvature = 2 * |cross| / (ab * bc * ca).
  (void)a;
  (void)b;
  (void)c;
  return -1.0;
}

// The fastest each point of a path can be taken: no faster than `max_speed`, and slow enough that the sideways acceleration,
// speed squared times curvature, stays within `max_lateral`. The two ends have no neighbour on one side and get `max_speed`.
inline std::vector<double> speed_limits(const std::vector<Vec2>& path, double max_speed, double max_lateral) {
  // TODO: start with max_speed at every point. For each interior point i (not the first or last):
  // k = curvature_at(path[i - 1], path[i], path[i + 1]); if k > 0 and max_lateral > 0 the limit is
  // min(max_speed, sqrt(max_lateral / k)).
  (void)path;
  (void)max_speed;
  (void)max_lateral;
  return std::vector<double>();
}

inline double distance_between(Vec2 a, Vec2 b) { return std::hypot(b.x - a.x, b.y - a.y); }

// A speed for each point that respects the limits and also the wheels: it never gains more than `max_accel` and never needs
// to lose more than `max_decel`. The first point starts at `start_speed` (or its limit if that is lower) and the last comes to rest.
// A forward pass for acceleration, then a backward pass from the stop for braking, using v squared = u squared + 2 a d.
inline std::vector<double> speed_profile(const std::vector<Vec2>& path, const std::vector<double>& limits, double max_accel,
                                         double max_decel, double start_speed) {
  // TODO: return an empty vector for an empty path or limits of a different size.
  // Start from the limits. The last speed is 0; the first is min(its limit, max(start_speed, 0)).
  // Forward: for i = 1 ...: v[i] = min(v[i], sqrt(v[i-1]^2 + 2 * max_accel * d)) with d the distance from point i-1 to i (distance_between).
  // Backward, from the end: v[i-1] = min(v[i-1], sqrt(v[i]^2 + 2 * max_decel * d)).
  (void)path;
  (void)limits;
  (void)max_accel;
  (void)max_decel;
  (void)start_speed;
  return std::vector<double>();
}

// Time to drive the path with these speeds: each piece takes its length over the mean of its two end speeds.
// A piece with both ends at rest cannot be driven, and makes the answer negative.
inline double travel_time(const std::vector<Vec2>& path, const std::vector<double>& speeds) {
  // TODO: 0 if the sizes differ or there are fewer than two points. For each piece, mean = (v[i-1] + v[i]) / 2;
  // if the mean is zero or less return -1; otherwise add distance / mean. Return the total.
  (void)path;
  (void)speeds;
  return -2.0;
}

#endif  // LESSON_SOLUTION_HPP
