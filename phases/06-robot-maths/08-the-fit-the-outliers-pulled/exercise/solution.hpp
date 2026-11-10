#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cmath>
#include <cstddef>
#include <vector>

#include <rc/core/random.hpp>

struct Point {
  double x = 0.0;
  double y = 0.0;
};

// A line a x + b y + c = 0 with a * a + b * b = 1.
//
// Written this way and not as y = m x + k, because a line that goes straight up has no slope. With a and b a unit
// vector, a x + b y + c is the signed distance from the line to the point (x, y), which is what a fit has to make small.
struct Line {
  double a = 0.0;
  double b = 1.0;
  double c = 0.0;
  bool valid = false;
};

// The distance from a point to a line.
inline double distance_to_line(const Line& line, const Point& p) { return std::fabs(line.a * p.x + line.b * p.y + line.c); }

// The angle of the line itself in degrees, between 0 and 180, which does not change if the line is described the other way round.
inline double line_angle_degrees(const Line& line) {
  double angle = std::atan2(line.a, -line.b) * 180.0 / 3.14159265358979323846;     // the direction along the line
  if (angle < 0.0) angle += 180.0;
  if (angle >= 180.0) angle -= 180.0;
  return angle;
}

// The smaller difference between two line angles, in degrees, where 179 and 1 are two degrees apart.
inline double angle_difference_degrees(double p, double q) {
  double d = std::fabs(p - q);
  if (d > 90.0) d = 180.0 - d;
  return d;
}

// Fit y = m x + k by making the vertical distances small. Given, so it can be compared. It cannot describe a vertical line, and it
// measures error in the wrong direction when the line is steep.
inline Line fit_vertical_distances(const std::vector<Point>& pts) {
  Line line;
  const double n = static_cast<double>(pts.size());
  if (pts.size() < 2) return line;
  double sx = 0, sy = 0, sxx = 0, sxy = 0;
  for (const Point& p : pts) {
    sx += p.x;
    sy += p.y;
    sxx += p.x * p.x;
    sxy += p.x * p.y;
  }
  const double denom = n * sxx - sx * sx;
  if (std::fabs(denom) < 1e-12) return line;
  const double m = (n * sxy - sx * sy) / denom;
  const double k = (sy - m * sx) / n;
  const double norm = std::sqrt(m * m + 1.0);
  line.a = m / norm;
  line.b = -1.0 / norm;
  line.c = k / norm;
  line.valid = true;
  return line;
}

// Fit a line by making the perpendicular distances small (total least squares).
inline Line fit_line(const std::vector<Point>& pts) {
  // TODO: fewer than 2 points gives an invalid line. The line goes through the centre (mx, my) of the points.
  // Sxx, Syy and Sxy are the sums of (x - mx)^2, (y - my)^2 and (x - mx)(y - my). If Sxx + Syy is under 1e-18 all
  // the points are one point: invalid. Otherwise theta = 0.5 * atan2(2 Sxy, Sxx - Syy) is the direction ALONG the
  // line, so a = -sin(theta), b = cos(theta), c = -(a mx + b my), valid = true.
  (void)pts;
  return Line();
}

// Fit a line that ignores wrong points.
inline Line ransac_line(const std::vector<Point>& pts, int tries, double threshold, unsigned seed) {
  // TODO: fewer than 2 points gives an invalid line. Make rc::core::Rng rng(seed). `tries` times: pick two
  // different points at random (rng.below(pts.size())), fit a line through the two with fit_line, skip it if it is not valid,
  // and collect the points within `threshold` of it (distance_to_line). Keep the largest set of such points.
  // At the end, if the best set has at least 2 points, return fit_line of that whole set, not of the two that chose it.
  (void)pts;
  (void)tries;
  (void)threshold;
  (void)seed;
  return Line();
}

#endif  // LESSON_SOLUTION_HPP
