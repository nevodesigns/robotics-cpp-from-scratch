#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cmath>
#include <vector>

#include <rc/core/random.hpp>
#include <rc/math/covariance.hpp>

using rc::math::Cov2;
using rc::math::Point2;

// Where the tool of a planar two link arm is, for the joint angles q1 (shoulder) and q2 (elbow, measured from the first link).
inline Point2 tool_position(double l1, double l2, double q1, double q2) {
  // TODO: x = l1 cos(q1) + l2 cos(q1 + q2), y = l1 sin(q1) + l2 sin(q1 + q2).
  (void)l1;
  (void)l2;
  (void)q1;
  (void)q2;
  return Point2();
}

// How a small change in each joint moves the tool: row 0 is the change in x, row 1 the change in y, and column 0 is the
// shoulder and column 1 the elbow.
struct Jac {
  double a = 0.0;   // dx / dq1
  double b = 0.0;   // dx / dq2
  double c = 0.0;   // dy / dq1
  double d = 0.0;   // dy / dq2
};

inline Jac jacobian(double l1, double l2, double q1, double q2) {
  // TODO: with s1 = sin(q1), c1 = cos(q1), s12 = sin(q1 + q2), c12 = cos(q1 + q2):
  //   a = -l1 s1 - l2 s12,  b = -l2 s12,  c = l1 c1 + l2 c12,  d = l2 c12.
  (void)l1;
  (void)l2;
  (void)q1;
  (void)q2;
  return Jac();
}

// The doubt at the tool, from the doubt in the joints, to first order: J C J'.
//
// If the joints are off by a small amount, the tool is off by J times that amount, and the covariance of J times something is J
// times its covariance times J transposed. `joint` is the covariance of (q1, q2): its xx is the variance of q1, its yy of q2, and its xy
// how the two errors move together.
inline Cov2 propagate(const Jac& j, const Cov2& joint) {
  // TODO: J C J'. With C = [[xx, xy], [xy, yy]] and J = [[a, b], [c, d]]:
  //   out.xx = a (a xx + b xy) + b (a xy + b yy)
  //   out.xy = c (a xx + b xy) + d (a xy + b yy)
  //   out.yy = c (c xx + d xy) + d (c xy + d yy)
  (void)j;
  (void)joint;
  return Cov2();
}

// Try it a lot of times: draw the joint angles at random around (q1, q2) with spreads s1 and s2, work out where the tool really is each
// time, and take the covariance of those positions. This needs no approximation, only patience.
inline Cov2 sampled_tool_covariance(double l1, double l2, double q1, double q2, double s1, double s2, int n, unsigned seed) {
  rc::core::Rng rng(seed);
  std::vector<Point2> pts;
  pts.reserve(static_cast<std::size_t>(n));
  for (int i = 0; i < n; ++i) pts.push_back(tool_position(l1, l2, q1 + s1 * rng.gaussian(), q2 + s2 * rng.gaussian()));
  return rc::math::covariance(pts);
}

#endif  // LESSON_SOLUTION_HPP
