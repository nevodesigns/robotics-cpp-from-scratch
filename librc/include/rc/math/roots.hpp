// rc/math/roots.hpp
//
// Where a function crosses zero, from lesson 06-10, graduated.
//
// Halving needs only a sign change and cannot fail: 40 steps for cos(x) - x over [0, 1] to 1e-12. Newton's method took 4 from a guess of 1.0, with the error going 2.6e-01, 1.1e-02, 2.8e-05, 1.7e-10, but nothing keeps its step small:
// on the cube root from 1 it ran off to 1.1e+12 in 40 steps, on the arctangent from 2 to -2.3e+10 in 5, and on x^2 - 2 from zero the slope is zero. Newton's steps inside a bracket, taken while they land inside it and
// at least halve the value of f, converged on all three: 4 steps on cos(x) - x, 5 on the arctangent over [-3.3, 5.1] (bisection 43), and 32 on the cube root, where every Newton step was refused and it was bisection throughout.

#ifndef RC_MATH_ROOTS_HPP
#define RC_MATH_ROOTS_HPP

#include <cmath>
#include <functional>

namespace rc {
namespace math {

using ScalarFunction = std::function<double(double)>;

// What a root finder reports: where, how many steps, and whether it got there.
struct Root {
  double x = 0.0;
  int iterations = 0;
  bool converged = false;
};

// Find where f crosses zero between a and b by halving.
//
// If f has opposite signs at the two ends, it must cross somewhere between them. Look at the middle: the half that
// still has opposite signs at its ends contains a crossing. Each step halves the interval, so it cannot fail and it
// needs about log2((b - a) / tol) steps: forty for an interval of 1 and a tolerance of 1e-12.
inline Root bisect(const ScalarFunction& f, double a, double b, double tol) {
  Root r;
  double fa = f(a);
  double fb = f(b);
  if (fa == 0.0) { r.x = a; r.converged = true; return r; }
  if (fb == 0.0) { r.x = b; r.converged = true; return r; }
  if ((fa > 0.0) == (fb > 0.0)) return r;           // the same sign at both ends: nothing says there is a crossing

  while (b - a > tol && r.iterations < 200) {
    const double m = 0.5 * (a + b);
    const double fm = f(m);
    ++r.iterations;
    if (fm == 0.0) {
      a = b = m;
      break;
    }
    if ((fm > 0.0) == (fa > 0.0)) {
      a = m;
      fa = fm;
    } else {
      b = m;
    }
  }
  r.x = 0.5 * (a + b);
  r.converged = (b - a) <= tol;
  return r;
}

// Newton's method: from a guess, go to where the tangent line crosses zero, and repeat.
//
// Fast when the guess is close: the number of correct digits roughly doubles each step. But nothing keeps the step
// small. Where the slope is nearly flat the step is enormous, and for some functions every step lands further from
// the root than the last. It reports failure if the slope is zero, the value stops being a finite number or runs away, or it is
// still not within `tol` after `max_iterations` steps.
inline Root newton(const ScalarFunction& f, const ScalarFunction& df, double x0, double tol, int max_iterations) {
  Root r;
  double x = x0;
  for (int i = 0; i < max_iterations; ++i) {
    const double fx = f(x);
    if (std::fabs(fx) <= tol) {
      r.x = x;
      r.iterations = i;
      r.converged = true;
      return r;
    }
    const double slope = df(x);
    if (std::fabs(slope) < 1e-14) {
      r.x = x;
      r.iterations = i;
      return r;
    }
    x -= fx / slope;
    if (!(std::fabs(x) < 1e12)) {
      r.x = x;
      r.iterations = i + 1;
      return r;
    }
  }
  r.x = x;
  r.iterations = max_iterations;
  r.converged = std::fabs(f(x)) <= tol;
  return r;
}

// Newton's steps while they help, halving when they do not.
//
// The root is kept between a and b, where f has opposite signs, as in bisection. From the current point take a Newton step;
// if it lands outside the bracket, or the value of f did not at least halve, take the middle of the bracket instead. The bracket shrinks every time
// either way, so it cannot run away, and near the root the Newton steps are accepted and it is fast.
inline Root safeguarded(const ScalarFunction& f, const ScalarFunction& df, double a, double b, double tol, int max_iterations) {
  Root r;
  double fa = f(a);
  double fb = f(b);
  if (fa == 0.0) { r.x = a; r.converged = true; return r; }
  if (fb == 0.0) { r.x = b; r.converged = true; return r; }
  if ((fa > 0.0) == (fb > 0.0)) return r;

  double x = 0.5 * (a + b);
  double fx = f(x);
  for (int i = 0; i < max_iterations; ++i) {
    if (std::fabs(fx) <= tol || (b - a) <= tol) {
      r.x = x;
      r.iterations = i;
      r.converged = true;
      return r;
    }
    // keep the bracket
    if ((fx > 0.0) == (fa > 0.0)) {
      a = x;
      fa = fx;
    } else {
      b = x;
      fb = fx;
    }

    double next = x;
    bool use_newton = false;
    const double slope = df(x);
    if (std::fabs(slope) >= 1e-14) {
      next = x - fx / slope;
      use_newton = next > a && next < b;
    }
    if (use_newton) {
      const double fnext = f(next);
      if (std::fabs(fnext) > 0.5 * std::fabs(fx)) use_newton = false;      // it did not help enough
    }
    if (!use_newton) next = 0.5 * (a + b);

    x = next;
    fx = f(x);
  }
  r.x = x;
  r.iterations = max_iterations;
  r.converged = std::fabs(fx) <= tol;
  return r;
}

}  // namespace math
}  // namespace rc

#endif  // RC_MATH_ROOTS_HPP
