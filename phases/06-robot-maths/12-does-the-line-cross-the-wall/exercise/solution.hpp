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
  // TODO: v = cross(a, b, c). scale = (the length from a to b) times (the length from a to c), using std::hypot.
  // If |v| <= 1e-12 * scale the point is on the line: return 0. Otherwise 1 if v is positive, else -1.
  (void)a;
  (void)b;
  (void)c;
  return 2;
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
  // TODO: o1 = orientation(p1, p2, q1), o2 = orientation(p1, p2, q2), o3 = orientation(q1, q2, p1),
  // o4 = orientation(q1, q2, p2). If o1 != o2 and o3 != o4 they cross: true. Otherwise the special cases: an end that
  // is on the other segment's line (orientation 0) AND between its ends (between(...)) is a touch: true for
  // (o1, p1-p2 and q1), (o2, p1-p2 and q2), (o3, q1-q2 and p1), (o4, q1-q2 and p2). Otherwise false.
  (void)p1;
  (void)p2;
  (void)q1;
  (void)q2;
  return true;
}

// The point on the segment a-b nearest to p: the foot of the perpendicular if it falls on the segment, otherwise the nearer end.
inline V closest_point_on_segment(const V& p, const V& a, const V& b) {
  // TODO: dx, dy from a to b and length2 = dx^2 + dy^2. If length2 is 0 the segment is a point: return a.
  // t = ((p - a) . (b - a)) / length2, held to [0, 1]. The answer is a + t * (b - a).
  (void)p;
  (void)b;
  return a;
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
  // TODO: fewer than 3 corners: false. For each edge from a = poly[j] to b = poly[i] (j is the corner before i,
  // starting with j = n - 1 and i = 0): if the point is on the edge (orientation(a, b, p) == 0 and between(a, b, p)) return true.
  // Count the edge if (a.y > p.y) != (b.y > p.y) AND p.x < a.x + (p.y - a.y) * (b.x - a.x) / (b.y - a.y): flip `inside`.
  // The half-open test (a.y > p.y) != (b.y > p.y) is what stops a corner on the ray being counted twice.
  (void)p;
  (void)poly;
  return false;
}

// The area of the polygon, positive if its corners go anticlockwise and negative if clockwise.
inline double polygon_area(const std::vector<V>& poly) {
  // TODO: twice the area is the sum over the edges, from corner j to corner i, of (xj * yi - xi * yj), with j the corner
  // before i and the first edge from the last corner to the first. Half of it is the area: positive anticlockwise.
  (void)poly;
  return -1.0;
}

#endif  // LESSON_SOLUTION_HPP
