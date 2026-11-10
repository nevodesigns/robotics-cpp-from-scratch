// rc/math/propagate.hpp
//
// The doubt in the joints, carried to the tool, from lesson 06-16, graduated.
//
// If the joints are off by a small amount, the tool is off by the Jacobian times it, and the covariance of the tool is J C J transposed. For a planar two link arm with links of 0.5 and 0.4 m, the shoulder at 0.4 and the elbow at 1.0 rad, the variance of x at the tool
// was 5.02e-05 by that formula and 5.03e-05 over 20000 random trials for joint errors of 0.01 rad, and 4.52e-02 against 4.00e-02 for errors of 0.3 rad, where the linear approximation fails. The 95 percent ellipse of the propagated doubt held 95.1 percent of 20000 trials. Near a
// straight arm the doubt is nearly all along one direction: the spread along and across the long axis was 13.97 and 5.73 mm with the elbow square, 19.69 and 0.20 mm with it at 0.05 rad, a ratio of 97.

#ifndef RC_MATH_PROPAGATE_HPP
#define RC_MATH_PROPAGATE_HPP

#include <cmath>
#include <vector>

#include <rc/core/random.hpp>
#include <rc/math/covariance.hpp>

namespace rc {
namespace math {

// Where the tool of a planar two link arm is, for the joint angles q1 (shoulder) and q2 (elbow, measured from the first link).
inline Point2 tool_position(double l1, double l2, double q1, double q2) {
  Point2 p;
  p.x = l1 * std::cos(q1) + l2 * std::cos(q1 + q2);
  p.y = l1 * std::sin(q1) + l2 * std::sin(q1 + q2);
  return p;
}

// How a small change in each joint moves the tool: row 0 is the change in x, row 1 the change in y, and column 0 is the
// shoulder and column 1 the elbow.
struct ToolJacobian {
  double a = 0.0;   // dx / dq1
  double b = 0.0;   // dx / dq2
  double c = 0.0;   // dy / dq1
  double d = 0.0;   // dy / dq2
};

inline ToolJacobian tool_jacobian(double l1, double l2, double q1, double q2) {
  ToolJacobian j;
  const double s1 = std::sin(q1);
  const double c1 = std::cos(q1);
  const double s12 = std::sin(q1 + q2);
  const double c12 = std::cos(q1 + q2);
  j.a = -l1 * s1 - l2 * s12;
  j.b = -l2 * s12;
  j.c = l1 * c1 + l2 * c12;
  j.d = l2 * c12;
  return j;
}

// The doubt at the tool, from the doubt in the joints, to first order: J C J'.
//
// If the joints are off by a small amount, the tool is off by J times that amount, and the covariance of J times something is J
// times its covariance times J transposed. `joint` is the covariance of (q1, q2): its xx is the variance of q1, its yy of q2, and its xy
// how the two errors move together.
inline Cov2 propagate(const ToolJacobian& j, const Cov2& joint) {
  Cov2 out;
  // J C: first row (a, b), second row (c, d), C = [[xx, xy], [xy, yy]].
  const double r00 = j.a * joint.xx + j.b * joint.xy;
  const double r01 = j.a * joint.xy + j.b * joint.yy;
  const double r10 = j.c * joint.xx + j.d * joint.xy;
  const double r11 = j.c * joint.xy + j.d * joint.yy;
  // (J C) J'
  out.xx = r00 * j.a + r01 * j.b;
  out.xy = r00 * j.c + r01 * j.d;
  out.yy = r10 * j.c + r11 * j.d;
  return out;
}

// Try it a lot of times: draw the joint angles at random around (q1, q2) with spreads s1 and s2, work out where the tool really is each
// time, and take the covariance of those positions. This needs no approximation, only patience.
inline Cov2 sampled_tool_covariance(double l1, double l2, double q1, double q2, double s1, double s2, int n, unsigned seed) {
  rc::core::Rng rng(seed);
  std::vector<Point2> pts;
  pts.reserve(static_cast<std::size_t>(n));
  for (int i = 0; i < n; ++i) pts.push_back(tool_position(l1, l2, q1 + s1 * rng.gaussian(), q2 + s2 * rng.gaussian()));
  return covariance(pts);
}

}  // namespace math
}  // namespace rc

#endif  // RC_MATH_PROPAGATE_HPP
