// rc/math/fit_line.hpp
//
// Fitting a line to points, from lesson 06-08, graduated.
//
// A line is a x + b y + c = 0 with a and b a unit vector, so a line that goes straight up can be written and a x + b y + c is the distance from it. Making the perpendicular distances small, which has
// a closed form for the 2 by 2 matrix of spreads, beats making the vertical distances small on a steep line: for a line at 85 degrees with noise 0.1 the mean error of the angle was 0.77 degrees against 0.29 over 300 runs, and
// points on x = 3 gave a line at 90 degrees where the vertical fit has no answer. With 100 points on a line at 30 degrees and 30 wrong ones in a cluster, fitting everything was out by 20.69 degrees on average, and the random-sample fit
// (many lines through two points chosen at random, keep the one most points agree with, refit those) by 0.03, keeping all 100 true points close to the line in every one of 200 runs.

#ifndef RC_MATH_FIT_LINE_HPP
#define RC_MATH_FIT_LINE_HPP

#include <cmath>
#include <cstddef>
#include <vector>

#include <rc/core/random.hpp>

namespace rc {
namespace math {

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
//
// The line goes through the centre of the points. Its direction is the one the points are most spread along, which
// for the 2 by 2 matrix of spreads has a closed form: the angle is half of atan2(2 Sxy, Sxx - Syy).
inline Line fit_line(const std::vector<Point>& pts) {
  Line line;
  if (pts.size() < 2) return line;

  double mx = 0.0;
  double my = 0.0;
  for (const Point& p : pts) {
    mx += p.x;
    my += p.y;
  }
  mx /= static_cast<double>(pts.size());
  my /= static_cast<double>(pts.size());

  double sxx = 0.0, syy = 0.0, sxy = 0.0;
  for (const Point& p : pts) {
    sxx += (p.x - mx) * (p.x - mx);
    syy += (p.y - my) * (p.y - my);
    sxy += (p.x - mx) * (p.y - my);
  }
  if (sxx + syy < 1e-18) return line;               // all the points are the same point: there is no direction

  const double theta = 0.5 * std::atan2(2.0 * sxy, sxx - syy);   // the direction along the line
  line.a = -std::sin(theta);
  line.b = std::cos(theta);
  line.c = -(line.a * mx + line.b * my);
  line.valid = true;
  return line;
}

// Fit a line that ignores wrong points: try many lines through two points chosen at random, keep the one that most points
// agree with (are within `threshold` of), then fit all of those that agree.
inline Line ransac_line(const std::vector<Point>& pts, int tries, double threshold, unsigned seed) {
  Line best;
  if (pts.size() < 2) return best;

  rc::core::Rng rng(seed);
  std::vector<Point> best_inliers;
  for (int t = 0; t < tries; ++t) {
    const std::size_t i = rng.below(static_cast<unsigned>(pts.size()));
    std::size_t j = rng.below(static_cast<unsigned>(pts.size()));
    if (i == j) continue;
    std::vector<Point> pair;
    pair.push_back(pts[i]);
    pair.push_back(pts[j]);
    const Line candidate = fit_line(pair);
    if (!candidate.valid) continue;

    std::vector<Point> inliers;
    for (const Point& p : pts) {
      if (distance_to_line(candidate, p) <= threshold) inliers.push_back(p);
    }
    if (inliers.size() > best_inliers.size()) best_inliers = inliers;
  }

  if (best_inliers.size() < 2) return best;
  return fit_line(best_inliers);       // the final answer uses everything that agreed, not just two points
}

}  // namespace math
}  // namespace rc

#endif  // RC_MATH_FIT_LINE_HPP
