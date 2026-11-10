#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cmath>
#include <cstddef>
#include <vector>

struct P {
  double x = 0.0;
  double y = 0.0;
};

inline P operator+(P a, P b) { return P{a.x + b.x, a.y + b.y}; }
inline P operator-(P a, P b) { return P{a.x - b.x, a.y - b.y}; }
inline P operator*(P a, double s) { return P{a.x * s, a.y * s}; }
inline double length(P a) { return std::hypot(a.x, a.y); }

// A point on the curve between p1 and p2, with p0 before them and p3 after, where u goes from 0 at p1 to 1 at p2.
//
// A Catmull-Rom curve: it passes through every waypoint, and where it arrives at one it leaves in the direction the
// neighbours suggest, so there is no corner. How the curve is timed along the way is `alpha`:
//   0.0  every waypoint takes the same time, however far apart: uniform
//   0.5  the time is the square root of the distance: centripetal
//   1.0  the time is the distance: chordal
// The curve is built from three rounds of blending between neighbouring points, weighted by those times.
inline P catmull_rom_point(P p0, P p1, P p2, P p3, double u, double alpha) {
  // TODO: the times. t0 = 0, t1 = t0 + length(p1 - p0)^alpha, t2 = t1 + length(p2 - p1)^alpha,
  // t3 = t2 + length(p3 - p2)^alpha (std::pow). If any of the three intervals is under 1e-12, return the straight
  // line p1 + (p2 - p1) * u. Otherwise t = t1 + u * (t2 - t1), and three rounds of blending:
  //   a1 = p0 (t1 - t)/(t1 - t0) + p1 (t - t0)/(t1 - t0)
  //   a2 = p1 (t2 - t)/(t2 - t1) + p2 (t - t1)/(t2 - t1)
  //   a3 = p2 (t3 - t)/(t3 - t2) + p3 (t - t2)/(t3 - t2)
  //   b1 = a1 (t2 - t)/(t2 - t0) + a2 (t - t0)/(t2 - t0)
  //   b2 = a2 (t3 - t)/(t3 - t1) + a3 (t - t1)/(t3 - t1)
  //   result = b1 (t2 - t)/(t2 - t1) + b2 (t - t1)/(t2 - t1)
  (void)p0;
  (void)p2;
  (void)p3;
  (void)u;
  (void)alpha;
  return p1;
}

// Points along the curve through all the waypoints, `per_segment` of them between each pair, ending exactly on the last.
//
// Waypoints that repeat are taken once. The curve needs a neighbour before the first waypoint and after the last, which
// do not exist, so each is made by reflecting the next one through the end: this keeps the end of the curve heading the
// way the first and last segments do.
inline std::vector<P> sample_path(const std::vector<P>& waypoints, int per_segment, double alpha) {
  std::vector<P> w;
  for (const P& p : waypoints) {
    if (w.empty() || length(p - w.back()) > 1e-12) w.push_back(p);
  }
  std::vector<P> path;
  if (w.size() < 2 || per_segment < 1) return w;

  const std::size_t n = w.size();
  for (std::size_t i = 0; i + 1 < n; ++i) {
    const P p0 = i > 0 ? w[i - 1] : w[0] * 2.0 - w[1];
    const P p3 = i + 2 < n ? w[i + 2] : w[n - 1] * 2.0 - w[n - 2];
    for (int k = 0; k < per_segment; ++k) {
      path.push_back(catmull_rom_point(p0, w[i], w[i + 1], p3, static_cast<double>(k) / per_segment, alpha));
    }
  }
  path.push_back(w[n - 1]);
  return path;
}

// How long the path is: the sum of the distances between neighbouring points.
inline double path_length(const std::vector<P>& path) {
  // TODO: the sum of length(path[i] - path[i - 1]) for i from 1.
  (void)path;
  return 0.0;
}

// The sharpest turn the path makes between one step and the next, in degrees: 0 for a straight path, 90 for a square corner, 180 for turning back on itself.
inline double max_turn_degrees(const std::vector<P>& path) {
  // TODO: for each three points in a row, a = path[i-1] - path[i-2] and b = path[i] - path[i-1]. Skip the
  // triple if either is shorter than 1e-12. The turn is acos of the dot product over the product of the lengths
  // (clamp that to [-1, 1] first), in degrees. Return the largest.
  (void)path;
  return 0.0;
}

#endif  // LESSON_SOLUTION_HPP
