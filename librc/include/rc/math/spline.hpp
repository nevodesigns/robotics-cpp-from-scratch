// rc/math/spline.hpp
//
// A smooth path through waypoints, from lesson 06-09, graduated.
//
// Straight lines between waypoints turn through the whole corner in one step: 90.0 degrees on a square route, where a Catmull-Rom curve through the same four waypoints turned at most 10.9 degrees in
// one of its steps, 20 to a segment, and was 30.93 long against 30.00. How the curve is timed along the way matters when the waypoints are unevenly spaced: for a route with spacing 10, 1 and 14.1 the uniform curve (alpha 0)
// turned back on itself with a sharpest step of 178.2 degrees, the centripetal one (alpha 0.5) 9.1 and the chordal one (alpha 1.0) 8.7. The curve passes through every waypoint whatever alpha is, waypoints on a straight line stay on it,
// and a repeated waypoint is taken once, since a zero distance would make a zero time interval to divide by.

#ifndef RC_MATH_SPLINE_HPP
#define RC_MATH_SPLINE_HPP

#include <cmath>
#include <cstddef>
#include <vector>

namespace rc {
namespace math {

struct Waypoint {
  double x = 0.0;
  double y = 0.0;
};

inline Waypoint operator+(Waypoint a, Waypoint b) { return Waypoint{a.x + b.x, a.y + b.y}; }
inline Waypoint operator-(Waypoint a, Waypoint b) { return Waypoint{a.x - b.x, a.y - b.y}; }
inline Waypoint operator*(Waypoint a, double s) { return Waypoint{a.x * s, a.y * s}; }
inline double length(Waypoint a) { return std::hypot(a.x, a.y); }

// A point on the curve between p1 and p2, with p0 before them and p3 after, where u goes from 0 at p1 to 1 at p2.
//
// A Catmull-Rom curve: it passes through every waypoint, and where it arrives at one it leaves in the direction the
// neighbours suggest, so there is no corner. How the curve is timed along the way is `alpha`:
//   0.0  every waypoint takes the same time, however far apart: uniform
//   0.5  the time is the square root of the distance: centripetal
//   1.0  the time is the distance: chordal
// The curve is built from three rounds of blending between neighbouring points, weighted by those times.
inline Waypoint catmull_rom_point(Waypoint p0, Waypoint p1, Waypoint p2, Waypoint p3, double u, double alpha) {
  const double t0 = 0.0;
  const double t1 = t0 + std::pow(length(p1 - p0), alpha);
  const double t2 = t1 + std::pow(length(p2 - p1), alpha);
  const double t3 = t2 + std::pow(length(p3 - p2), alpha);

  // If two waypoints coincide a time interval is zero and the blending would divide by it: go straight.
  if (t1 - t0 < 1e-12 || t2 - t1 < 1e-12 || t3 - t2 < 1e-12) return p1 + (p2 - p1) * u;

  const double t = t1 + u * (t2 - t1);

  const Waypoint a1 = p0 * ((t1 - t) / (t1 - t0)) + p1 * ((t - t0) / (t1 - t0));
  const Waypoint a2 = p1 * ((t2 - t) / (t2 - t1)) + p2 * ((t - t1) / (t2 - t1));
  const Waypoint a3 = p2 * ((t3 - t) / (t3 - t2)) + p3 * ((t - t2) / (t3 - t2));

  const Waypoint b1 = a1 * ((t2 - t) / (t2 - t0)) + a2 * ((t - t0) / (t2 - t0));
  const Waypoint b2 = a2 * ((t3 - t) / (t3 - t1)) + a3 * ((t - t1) / (t3 - t1));

  return b1 * ((t2 - t) / (t2 - t1)) + b2 * ((t - t1) / (t2 - t1));
}

// Points along the curve through all the waypoints, `per_segment` of them between each pair, ending exactly on the last.
//
// Waypoints that repeat are taken once. The curve needs a neighbour before the first waypoint and after the last, which
// do not exist, so each is made by reflecting the next one through the end: this keeps the end of the curve heading the
// way the first and last segments do.
inline std::vector<Waypoint> sample_path(const std::vector<Waypoint>& waypoints, int per_segment, double alpha) {
  std::vector<Waypoint> w;
  for (const Waypoint& p : waypoints) {
    if (w.empty() || length(p - w.back()) > 1e-12) w.push_back(p);
  }
  std::vector<Waypoint> path;
  if (w.size() < 2 || per_segment < 1) return w;

  const std::size_t n = w.size();
  for (std::size_t i = 0; i + 1 < n; ++i) {
    const Waypoint p0 = i > 0 ? w[i - 1] : w[0] * 2.0 - w[1];
    const Waypoint p3 = i + 2 < n ? w[i + 2] : w[n - 1] * 2.0 - w[n - 2];
    for (int k = 0; k < per_segment; ++k) {
      path.push_back(catmull_rom_point(p0, w[i], w[i + 1], p3, static_cast<double>(k) / per_segment, alpha));
    }
  }
  path.push_back(w[n - 1]);
  return path;
}

// How long the path is: the sum of the distances between neighbouring points.
inline double path_length(const std::vector<Waypoint>& path) {
  double total = 0.0;
  for (std::size_t i = 1; i < path.size(); ++i) total += length(path[i] - path[i - 1]);
  return total;
}

// The sharpest turn the path makes between one step and the next, in degrees: 0 for a straight path, 90 for a square corner, 180 for turning back on itself.
inline double max_turn_degrees(const std::vector<Waypoint>& path) {
  double worst = 0.0;
  for (std::size_t i = 2; i < path.size(); ++i) {
    const Waypoint a = path[i - 1] - path[i - 2];
    const Waypoint b = path[i] - path[i - 1];
    const double la = length(a);
    const double lb = length(b);
    if (la < 1e-12 || lb < 1e-12) continue;
    double c = (a.x * b.x + a.y * b.y) / (la * lb);
    if (c > 1.0) c = 1.0;
    if (c < -1.0) c = -1.0;
    const double degrees = std::acos(c) * 180.0 / 3.14159265358979323846;
    if (degrees > worst) worst = degrees;
  }
  return worst;
}

}  // namespace math
}  // namespace rc

#endif  // RC_MATH_SPLINE_HPP
