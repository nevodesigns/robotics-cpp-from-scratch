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
  // TODO: fewer than two points give a Cov2 of zeros. Otherwise take the mean (mean_of), and add up
  // (x - mx)^2, (x - mx)(y - my) and (y - my)^2 over all the points, then divide each sum by (count - 1).
  (void)pts;
  return Cov2();
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
  // TODO: mean = (xx + yy) / 2 and radius = sqrt(((xx - yy) / 2)^2 + xy^2). major = mean + radius and
  // minor = mean - radius. The angle of the long axis is 0.5 * atan2(2 xy, xx - yy), turned into degrees, and brought
  // into [0, 180) by adding 180 if it is negative.
  (void)c;
  return Axes();
}

// How far a point is from the centre, measured in the units of the cloud's own spread, squared.
//
// The squared Mahalanobis distance: (p - m)' C^-1 (p - m), which for a 2 by 2 matrix needs only its inverse written out. A
// point one spread away in any direction has a distance of 1, however the cloud is stretched or tilted. If the cloud has no
// width in some direction (the matrix has no inverse) the distance is infinite for any point off its line.
inline double mahalanobis_squared(const Pt& p, const Pt& centre, const Cov2& c) {
  // TODO: det = xx * yy - xy^2. If det is under 1e-18 the cloud has no width in some direction: the distance is 0
  // for the centre itself and infinity for any other point (std::numeric_limits<double>::infinity()).
  // Otherwise, with dx and dy the offsets from the centre, (yy dx^2 - 2 xy dx dy + xx dy^2) / det.
  (void)p;
  (void)centre;
  (void)c;
  return -1.0;
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
