#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cmath>
#include <cstddef>
#include <limits>
#include <vector>

#include <rc/core/random.hpp>

struct Pt {
  double x = 0.0;
  double y = 0.0;
};

// How a cloud of points is spread: the variance in x, the variance in y, and how much x and y move together.
struct Cov2 {
  double xx = 0.0;
  double xy = 0.0;
  double yy = 0.0;
};

inline Pt mean_of(const std::vector<Pt>& pts) {
  Pt m;
  if (pts.empty()) return m;
  for (const Pt& p : pts) {
    m.x += p.x;
    m.y += p.y;
  }
  m.x /= static_cast<double>(pts.size());
  m.y /= static_cast<double>(pts.size());
  return m;
}

// The sample covariance: sums of products of the distances from the mean, divided by one less than the number of points.
// Fewer than two points give no spread at all.
inline Cov2 covariance(const std::vector<Pt>& pts) {
  Cov2 c;
  if (pts.size() < 2) return c;
  const Pt m = mean_of(pts);
  for (const Pt& p : pts) {
    c.xx += (p.x - m.x) * (p.x - m.x);
    c.xy += (p.x - m.x) * (p.y - m.y);
    c.yy += (p.y - m.y) * (p.y - m.y);
  }
  const double n1 = static_cast<double>(pts.size() - 1);
  c.xx /= n1;
  c.xy /= n1;
  c.yy /= n1;
  return c;
}

// The directions the cloud is most and least spread along, and how much: the eigenvalues and the angle of the long axis.
struct Axes {
  double major = 0.0;           // the larger variance, along the long axis
  double minor = 0.0;           // the smaller, across it
  double angle_degrees = 0.0;   // the direction of the long axis, from 0 up to but not including 180
};

// A 2 by 2 symmetric matrix has its eigenvalues in closed form: the mean of the two variances plus or minus a radius,
// and the angle of the long axis is half the angle of (xx - yy, 2 xy).
inline Axes principal_axes(const Cov2& c) {
  Axes a;
  const double mean = 0.5 * (c.xx + c.yy);
  const double radius = std::sqrt(0.25 * (c.xx - c.yy) * (c.xx - c.yy) + c.xy * c.xy);
  a.major = mean + radius;
  a.minor = mean - radius;
  double angle = 0.5 * std::atan2(2.0 * c.xy, c.xx - c.yy) * 180.0 / 3.14159265358979323846;
  if (angle < 0.0) angle += 180.0;
  if (angle >= 180.0) angle -= 180.0;
  a.angle_degrees = angle;
  return a;
}

// How far a point is from the centre, measured in the units of the cloud's own spread, squared.
//
// The squared Mahalanobis distance: (p - m)' C^-1 (p - m), which for a 2 by 2 matrix needs only its inverse written out. A
// point one spread away in any direction has a distance of 1, however the cloud is stretched or tilted. If the cloud has no
// width in some direction (the matrix has no inverse) the distance is infinite for any point off its line.
inline double mahalanobis_squared(const Pt& p, const Pt& centre, const Cov2& c) {
  const double det = c.xx * c.yy - c.xy * c.xy;
  const double dx = p.x - centre.x;
  const double dy = p.y - centre.y;
  if (det < 1e-18) {
    return (dx == 0.0 && dy == 0.0) ? 0.0 : std::numeric_limits<double>::infinity();
  }
  return (c.yy * dx * dx - 2.0 * c.xy * dx * dy + c.xx * dy * dy) / det;
}

// 5.991 is the squared distance that holds 95 percent of a bell shaped cloud in two dimensions.
const double kInside95 = 5.991464547107979;

inline bool inside_ellipse(const Pt& p, const Pt& centre, const Cov2& c, double squared_limit) {
  return mahalanobis_squared(p, centre, c) <= squared_limit;
}

// Points from a bell shaped cloud with spreads sx and sy and a correlation rho between them, centred on (cx, cy).
inline std::vector<Pt> correlated_cloud(unsigned seed, int n, double cx, double cy, double sx, double sy, double rho) {
  rc::core::Rng rng(seed);
  std::vector<Pt> pts;
  const double k = std::sqrt(1.0 - rho * rho);
  for (int i = 0; i < n; ++i) {
    const double z1 = rng.gaussian();
    const double z2 = rng.gaussian();
    pts.push_back(Pt{cx + sx * z1, cy + sy * (rho * z1 + k * z2)});
  }
  return pts;
}

#endif  // LESSON_SOLUTION_HPP
