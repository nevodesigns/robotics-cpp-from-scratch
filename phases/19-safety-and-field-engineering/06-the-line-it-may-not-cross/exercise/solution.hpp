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
  // TODO: project p on ab: t = ((p-a) . (b-a)) / |b-a|^2 (zero if a == b), clamp t to 0..1, the nearest point is a + t (b-a); the distance to it (std::hypot)
  (void)p;
  (void)a;
  (void)b;
  return 0.0;
}

// The distance from p to the nearest edge of a closed polygon (the last vertex joins the first). Infinity for no vertices.
inline double distance_to_edge(const Point& p, const std::vector<Point>& area) {
  // TODO: the smallest distance_to_segment over the edges (i to i+1, the last joining the first); infinity if there are none
  (void)p;
  (void)area;
  return INFINITY;
}

// Is p strictly inside the polygon? A point on the boundary is NOT inside: on the line is where the robot is not allowed to be.
// Ray casting to the right, counting an edge when exactly one of its ends is above the point's y (half open), which counts a vertex on the ray once and not twice or not at all.
inline bool inside(const Point& p, const std::vector<Point>& area) {
  // TODO: false for fewer than 3 vertices or a NaN point, and false on the boundary (distance to the edge under 1e-9).
  // Otherwise cast a ray to the right: an edge counts only when exactly one end is above p.y, (a.y > p.y) != (b.y > p.y), and the point is left of where the edge crosses that height; an odd count is inside
  (void)p;
  (void)area;
  return false;
}

enum class FenceState { inside, warning, outside, unknown };

// Where the robot is with regard to the fence. The robot must stop while it still has the room to: it is `outside` (stop) when it is not strictly inside, or nearer the edge than stop_buffer;
// `warning` when it is nearer than stop_buffer + warn_extra; `unknown` when its position is not a number (and that is a stop, too).
inline FenceState fence_state(const std::vector<Point>& area, const Point& p, double stop_buffer, double warn_extra) {
  // TODO: unknown for a NaN position; outside if not inside(); else with d the distance to the edge: outside if d < stop_buffer, warning if d < stop_buffer + warn_extra, else inside
  (void)area;
  (void)p;
  (void)stop_buffer;
  (void)warn_extra;
  return FenceState::unknown;
}

// How far from the edge the robot must stop, given how far it goes after the stop is ordered (its stopping distance) and how wrong its position may be: dead reckoning drifts by a fraction of the distance travelled since the last fix.
inline double required_buffer(double stopping_distance, double drift_per_metre, double metres_since_fix) {
  // TODO: stopping_distance + drift * metres, neither of the last two below zero
  (void)stopping_distance;
  (void)drift_per_metre;
  (void)metres_since_fix;
  return 0.0;
}

// The speed allowed by the distance to the nearest edge, once the position's uncertainty is taken off it: the same rule as for a sensor's range (lesson 19-05). Zero when outside or when the position is not a number.
inline double fence_speed(const std::vector<Point>& area, const Point& p, double position_error, double decel, double latency, double margin) {
  // TODO: zero if not inside; otherwise rc::safety::max_speed(distance to the edge minus the position error (not below 0), decel, latency, margin)
  (void)area;
  (void)p;
  (void)position_error;
  (void)decel;
  (void)latency;
  (void)margin;
  return 0.0;
}

#endif  // LESSON_SOLUTION_HPP
