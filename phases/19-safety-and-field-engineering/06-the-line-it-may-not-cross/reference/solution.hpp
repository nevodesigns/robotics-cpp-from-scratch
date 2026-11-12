#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <algorithm>
#include <cmath>
#include <rc/safety/stopping.hpp>
#include <vector>

struct Point {
  double x = 0.0;
  double y = 0.0;
};

// The shortest distance from p to the segment ab (not to the line through it: past the end of a segment the nearest point is the end).
inline double distance_to_segment(const Point& p, const Point& a, const Point& b) {
  const double dx = b.x - a.x;
  const double dy = b.y - a.y;
  const double len2 = dx * dx + dy * dy;
  double t = len2 > 0.0 ? ((p.x - a.x) * dx + (p.y - a.y) * dy) / len2 : 0.0;
  t = std::max(0.0, std::min(1.0, t));
  const double cx = a.x + t * dx;
  const double cy = a.y + t * dy;
  return std::hypot(p.x - cx, p.y - cy);
}

// The distance from p to the nearest edge of a closed polygon (the last vertex joins the first). Infinity for no vertices.
inline double distance_to_edge(const Point& p, const std::vector<Point>& area) {
  double best = INFINITY;
  const std::size_t n = area.size();
  for (std::size_t i = 0; i < n; ++i) best = std::min(best, distance_to_segment(p, area[i], area[(i + 1) % n]));
  return best;
}

// Is p strictly inside the polygon? A point on the boundary is NOT inside: on the line is where the robot is not allowed to be.
// Ray casting to the right, counting an edge when exactly one of its ends is above the point's y (half open), which counts a vertex on the ray once and not twice or not at all.
inline bool inside(const Point& p, const std::vector<Point>& area) {
  const std::size_t n = area.size();
  if (n < 3 || std::isnan(p.x) || std::isnan(p.y)) return false;
  if (distance_to_edge(p, area) < 1e-9) return false;
  bool in = false;
  for (std::size_t i = 0; i < n; ++i) {
    const Point& a = area[i];
    const Point& b = area[(i + 1) % n];
    if ((a.y > p.y) != (b.y > p.y)) {
      const double x_at = (b.x - a.x) * (p.y - a.y) / (b.y - a.y) + a.x;
      if (p.x < x_at) in = !in;
    }
  }
  return in;
}

enum class FenceState { inside, warning, outside, unknown };

// Where the robot is with regard to the fence. The robot must stop while it still has the room to: it is `outside` (stop) when it is not strictly inside, or nearer the edge than stop_buffer;
// `warning` when it is nearer than stop_buffer + warn_extra; `unknown` when its position is not a number (and that is a stop, too).
inline FenceState fence_state(const std::vector<Point>& area, const Point& p, double stop_buffer, double warn_extra) {
  if (std::isnan(p.x) || std::isnan(p.y)) return FenceState::unknown;
  if (!inside(p, area)) return FenceState::outside;
  const double d = distance_to_edge(p, area);
  if (d < stop_buffer) return FenceState::outside;
  if (d < stop_buffer + warn_extra) return FenceState::warning;
  return FenceState::inside;
}

// How far from the edge the robot must stop, given how far it goes after the stop is ordered (its stopping distance) and how wrong its position may be: dead reckoning drifts by a fraction of the distance travelled since the last fix.
inline double required_buffer(double stopping_distance, double drift_per_metre, double metres_since_fix) {
  return stopping_distance + std::max(drift_per_metre, 0.0) * std::max(metres_since_fix, 0.0);
}

// The speed allowed by the distance to the nearest edge, once the position's uncertainty is taken off it: the same rule as for a sensor's range (lesson 19-05). Zero when outside or when the position is not a number.
inline double fence_speed(const std::vector<Point>& area, const Point& p, double position_error, double decel, double latency, double margin) {
  if (!inside(p, area)) return 0.0;
  return rc::safety::max_speed(distance_to_edge(p, area) - std::max(position_error, 0.0), decel, latency, margin);
}

#endif  // LESSON_SOLUTION_HPP
