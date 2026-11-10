#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cmath>
#include <cstddef>
#include <vector>

#include <rc/core/random.hpp>
#include <rc/math/linear.hpp>

struct Pt {
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
inline Circle fit_circle_algebraic(const std::vector<Pt>& pts) {
  // TODO: fewer than 3 points give an invalid circle. With z = x^2 + y^2 for each point, add up
  // sx = sum x, sy = sum y, sxx = sum x^2, syy = sum y^2, sxy = sum x y, sxz = sum x z, syz = sum y z, sz = sum z,
  // and n. The normal equations for (D, E, F) are
  //   [sxx sxy sx] [D]   [-sxz]
  //   [sxy syy sy] [E] = [-syz]
  //   [sx  sy  n ] [F]   [-sz ]
  // Build that as an rc::math::Matrix (3 by 3) and solve it with rc::math::solve(a, rhs, ok); if not ok the circle is
  // invalid. The centre is (-D / 2, -E / 2) and the radius squared is cx^2 + cy^2 - F; if that is not above 0 the circle is invalid. Otherwise
  // the radius is its square root and the circle is valid.
  (void)pts;
  return Circle();
}

// Improve a circle by minimising the real distances from the points to it, by Gauss-Newton.
//
// The residual of a point is its distance from the centre minus the radius. Its derivative with respect to the centre is a unit vector
// pointing away from the point, and with respect to the radius it is -1. Each step solves a 3 by 3 system of normal equations for
// the change to make, and a handful of steps is enough from a good start.
inline Circle refine_circle(const std::vector<Pt>& pts, Circle c, int steps) {
  // TODO: if c is not valid return it. Repeat `steps` times: for each point, dx = x - cx, dy = y - cy, rho = hypot(dx, dy)
  // (skip the point if rho < 1e-12), the derivatives of the residual are j = (-dx/rho, -dy/rho, -1) and the residual
  // is rho - r. Add j j' into a 3 by 3 matrix and j * residual into a vector of 3. Solve (that matrix) step = -(that vector)
  // with rc::math::solve; if it fails stop. Add step[0], step[1], step[2] to cx, cy and r.
  (void)pts;
  (void)steps;
  return c;
}

// Points on an arc of a circle: `n` of them spread evenly in angle over `arc_degrees`, with noise of `sigma` on each coordinate.
inline std::vector<Pt> arc_points(unsigned seed, int n, double cx, double cy, double r, double arc_degrees, double sigma) {
  rc::core::Rng rng(seed);
  std::vector<Pt> pts;
  for (int i = 0; i < n; ++i) {
    const double t = (arc_degrees * 3.14159265358979323846 / 180.0) * static_cast<double>(i) / static_cast<double>(n);
    pts.push_back(Pt{cx + r * std::cos(t) + sigma * rng.gaussian(), cy + r * std::sin(t) + sigma * rng.gaussian()});
  }
  return pts;
}

#endif  // LESSON_SOLUTION_HPP
