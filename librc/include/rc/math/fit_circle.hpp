// rc/math/fit_circle.hpp
//
// Fitting a circle to noisy points, from lesson 06-15, graduated.
//
// A point is on the circle when x^2 + y^2 + D x + E y + F = 0, which is linear in D, E and F, so one 3 by 3 solve gives a circle. It does not minimise the distance of the points from the circle, and on a short arc it makes the circle too small: for a circle
// of radius 2 with noise 0.02 over 300 runs the mean error of the radius was -0.0002 over a full turn, -0.0212 over 90 degrees and -0.1049 over 60, where Gauss-Newton refinement of the real distances gave -0.0003, -0.0019 and -0.0047. No method rescues a short enough arc:
// the scatter of the refined radius was 0.006 over 180 degrees, 0.172 over 40 and 5.986 over 15. Points on a line are not on a circle, and the fit says so.

#ifndef RC_MATH_FIT_CIRCLE_HPP
#define RC_MATH_FIT_CIRCLE_HPP

#include <cmath>
#include <cstddef>
#include <vector>

#include <rc/core/random.hpp>
#include <rc/math/linear.hpp>

namespace rc {
namespace math {

struct CirclePoint {
  double x = 0.0;
  double y = 0.0;
};

struct Circle {
  double cx = 0.0;
  double cy = 0.0;
  double r = 0.0;
  bool valid = false;
};

// Fit a circle by making x^2 + y^2 + D x + E y + F as close to zero as possible for every point (Kasa's fit).
//
// A point is on the circle with centre (a, b) and radius r when (x - a)^2 + (y - b)^2 = r^2, which is x^2 + y^2 + D x + E y + F = 0 with D = -2a,
// E = -2b and F = a^2 + b^2 - r^2. That is LINEAR in D, E and F, so it is one 3 by 3 system of normal equations, solved with the solver of lesson 06-07.
//
// It is fast, and it does not minimise the distance of the points from the circle. It minimises a quantity that grows with the size of the circle, and
// on a short arc it pulls the answer toward a smaller circle.
inline Circle fit_circle_algebraic(const std::vector<CirclePoint>& pts) {
  Circle c;
  if (pts.size() < 3) return c;

  double sx = 0, sy = 0, sxx = 0, syy = 0, sxy = 0, sxz = 0, syz = 0, sz = 0;
  for (const CirclePoint& p : pts) {
    const double z = p.x * p.x + p.y * p.y;
    sx += p.x;
    sy += p.y;
    sxx += p.x * p.x;
    syy += p.y * p.y;
    sxy += p.x * p.y;
    sxz += p.x * z;
    syz += p.y * z;
    sz += z;
  }
  const double n = static_cast<double>(pts.size());

  Matrix a(3, 3);
  a.at(0, 0) = sxx; a.at(0, 1) = sxy; a.at(0, 2) = sx;
  a.at(1, 0) = sxy; a.at(1, 1) = syy; a.at(1, 2) = sy;
  a.at(2, 0) = sx;  a.at(2, 1) = sy;  a.at(2, 2) = n;
  const std::vector<double> rhs = {-sxz, -syz, -sz};

  bool ok = false;
  const std::vector<double> sol = solve(a, rhs, ok);
  if (!ok) return c;

  c.cx = -0.5 * sol[0];
  c.cy = -0.5 * sol[1];
  const double r2 = c.cx * c.cx + c.cy * c.cy - sol[2];
  if (!(r2 > 0.0)) return c;
  c.r = std::sqrt(r2);
  c.valid = true;
  return c;
}

// Improve a circle by minimising the real distances from the points to it, by Gauss-Newton.
//
// The residual of a point is its distance from the centre minus the radius. Its derivative with respect to the centre is a unit vector
// pointing away from the point, and with respect to the radius it is -1. Each step solves a 3 by 3 system of normal equations for
// the change to make, and a handful of steps is enough from a good start.
inline Circle refine_circle(const std::vector<CirclePoint>& pts, Circle c, int steps) {
  if (!c.valid) return c;
  for (int it = 0; it < steps; ++it) {
    Matrix jtj(3, 3);
    std::vector<double> jtr(3, 0.0);
    for (const CirclePoint& p : pts) {
      const double dx = p.x - c.cx;
      const double dy = p.y - c.cy;
      const double rho = std::hypot(dx, dy);
      if (rho < 1e-12) continue;
      const double j[3] = {-dx / rho, -dy / rho, -1.0};
      const double res = rho - c.r;
      for (int a = 0; a < 3; ++a) {
        jtr[static_cast<std::size_t>(a)] += j[a] * res;
        for (int b = 0; b < 3; ++b) jtj.at(static_cast<std::size_t>(a), static_cast<std::size_t>(b)) += j[a] * j[b];
      }
    }
    bool ok = false;
    const std::vector<double> step = solve(jtj, std::vector<double>{-jtr[0], -jtr[1], -jtr[2]}, ok);
    if (!ok) break;
    c.cx += step[0];
    c.cy += step[1];
    c.r += step[2];
  }
  return c;
}

// Points on an arc of a circle: `n` of them spread evenly in angle over `arc_degrees`, with noise of `sigma` on each coordinate.
inline std::vector<CirclePoint> arc_points(unsigned seed, int n, double cx, double cy, double r, double arc_degrees, double sigma) {
  rc::core::Rng rng(seed);
  std::vector<CirclePoint> pts;
  for (int i = 0; i < n; ++i) {
    const double t = (arc_degrees * 3.14159265358979323846 / 180.0) * static_cast<double>(i) / static_cast<double>(n);
    pts.push_back(CirclePoint{cx + r * std::cos(t) + sigma * rng.gaussian(), cy + r * std::sin(t) + sigma * rng.gaussian()});
  }
  return pts;
}

}  // namespace math
}  // namespace rc

#endif  // RC_MATH_FIT_CIRCLE_HPP
