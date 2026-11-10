#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <vector>

struct V {
  double x = 0.0;
  double y = 0.0;
};

// Twice the signed area of the triangle o, a, b: positive if b is to the left of the way from o to a.
inline double cross(const V& o, const V& a, const V& b) { return (a.x - o.x) * (b.y - o.y) - (a.y - o.y) * (b.x - o.x); }

// Which side of the line from a to b the point c is on: 1 for the left, -1 for the right, 0 if it is on the line.
//
// "On the line" cannot be tested as cross == 0. Three points that are in a line on paper are not exactly in a line once they are
// stored as doubles, and the cross product comes out as a tiny number of either sign. The test compares it with the size of
// the triangle's sides, so it means "closer to the line than 1e-12 of the way's length".
inline int orientation(const V& a, const V& b, const V& c) {
  const double v = cross(a, b, c);
  const double scale = std::hypot(b.x - a.x, b.y - a.y) * std::hypot(c.x - a.x, c.y - a.y);
  if (std::fabs(v) <= 1e-12 * scale) return 0;
  return v > 0.0 ? 1 : -1;
}

// Is p, which is on the line through a and b, between them?
inline bool between(const V& a, const V& b, const V& p) {
  return std::min(a.x, b.x) <= p.x && p.x <= std::max(a.x, b.x) && std::min(a.y, b.y) <= p.y && p.y <= std::max(a.y, b.y);
}

// Do the segments p1-p2 and q1-q2 share a point, an end included?
//
// If the ends of each segment are on opposite sides of the other's line, they cross. The rest are the special cases: an end
// of one that lies on the other (a touch, a T), and segments on one line that overlap or do not.
inline bool segments_intersect(const V& p1, const V& p2, const V& q1, const V& q2) {
  const int o1 = orientation(p1, p2, q1);
  const int o2 = orientation(p1, p2, q2);
  const int o3 = orientation(q1, q2, p1);
  const int o4 = orientation(q1, q2, p2);

  if (o1 != o2 && o3 != o4) return true;

  if (o1 == 0 && between(p1, p2, q1)) return true;
  if (o2 == 0 && between(p1, p2, q2)) return true;
  if (o3 == 0 && between(q1, q2, p1)) return true;
  if (o4 == 0 && between(q1, q2, p2)) return true;
  return false;
}

// The point on the segment a-b nearest to p: the foot of the perpendicular if it falls on the segment, otherwise the nearer end.
inline V closest_point_on_segment(const V& p, const V& a, const V& b) {
  const double dx = b.x - a.x;
  const double dy = b.y - a.y;
  const double length2 = dx * dx + dy * dy;
  if (length2 == 0.0) return a;                        // a segment of no length is a point
  double t = ((p.x - a.x) * dx + (p.y - a.y) * dy) / length2;
  t = std::max(0.0, std::min(1.0, t));
  return V{a.x + t * dx, a.y + t * dy};
}

inline double distance_to_segment(const V& p, const V& a, const V& b) {
  const V c = closest_point_on_segment(p, a, b);
  return std::hypot(p.x - c.x, p.y - c.y);
}

// Is the point inside the polygon, or on its edge?
//
// Draw a ray from the point to the right and count the edges it crosses: an odd number means inside. The ray can pass
// exactly through a corner, and then it would cross two edges at once and be counted twice. The fix is a rule that counts an edge only if
// one end is strictly above the ray and the other is not, so a corner on the ray belongs to exactly one of the two edges that meet there.
inline bool point_in_polygon(const V& p, const std::vector<V>& poly) {
  const std::size_t n = poly.size();
  if (n < 3) return false;

  bool inside = false;
  for (std::size_t i = 0, j = n - 1; i < n; j = i++) {
    const V& a = poly[j];
    const V& b = poly[i];
    if (orientation(a, b, p) == 0 && between(a, b, p)) return true;            // on an edge counts as inside
    if ((a.y > p.y) != (b.y > p.y)) {
      const double x_at_ray = a.x + (p.y - a.y) * (b.x - a.x) / (b.y - a.y);
      if (p.x < x_at_ray) inside = !inside;
    }
  }
  return inside;
}

// The area of the polygon, positive if its corners go anticlockwise and negative if clockwise.
inline double polygon_area(const std::vector<V>& poly) {
  double twice = 0.0;
  const std::size_t n = poly.size();
  for (std::size_t i = 0, j = n - 1; i < n; j = i++) twice += poly[j].x * poly[i].y - poly[i].x * poly[j].y;
  return 0.5 * twice;
}

#endif  // LESSON_SOLUTION_HPP
