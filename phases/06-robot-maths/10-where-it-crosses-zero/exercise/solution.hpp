#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cmath>
#include <functional>

using Function = std::function<double(double)>;

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
inline Root bisect(const Function& f, double a, double b, double tol) {
  // TODO: fa = f(a), fb = f(b). If one of them is exactly zero that end is the root: x set, converged true.
  // If fa and fb have the same sign there may be no crossing: return the Root as it is (converged false).
  // Otherwise, while b - a > tol and fewer than 200 iterations: m = (a + b) / 2, fm = f(m), count an iteration.
  // If fm is zero, the root is m. If fm has the same sign as fa, the crossing is in the upper half: a = m,
  // fa = fm; else b = m. At the end x = (a + b) / 2 and converged = (b - a) <= tol.
  (void)f;
  (void)a;
  (void)b;
  (void)tol;
  return Root();
}

// Newton's method: from a guess, go to where the tangent line crosses zero, and repeat.
//
// Fast when the guess is close: the number of correct digits roughly doubles each step. But nothing keeps the step
// small. Where the slope is nearly flat the step is enormous, and for some functions every step lands further from
// the root than the last. It reports failure if the slope is zero, the value stops being a finite number or runs away, or it is
// still not within `tol` after `max_iterations` steps.
inline Root newton(const Function& f, const Function& df, double x0, double tol, int max_iterations) {
  // TODO: x = x0. For i from 0 to max_iterations - 1: fx = f(x). If |fx| <= tol, report x, iterations = i and
  // converged. slope = df(x): if |slope| < 1e-14 stop with converged false. Take the step x -= fx / slope. If x is
  // not finite or its size is not below 1e12 (write !(std::fabs(x) < 1e12)) stop with converged false and
  // iterations i + 1. After the loop report x, iterations = max_iterations and converged = |f(x)| <= tol.
  (void)f;
  (void)df;
  (void)x0;
  (void)tol;
  (void)max_iterations;
  return Root();
}

// Newton's steps while they help, halving when they do not.
inline Root safeguarded(const Function& f, const Function& df, double a, double b, double tol, int max_iterations) {
  // TODO: as bisect: an end that is exactly zero is the root, and the same sign at both ends is not converged.
  // Keep the root in a bracket [a, b] with f of opposite signs at the ends. Start at x = (a + b) / 2. Each step:
  //  - if |f(x)| <= tol or b - a <= tol, report x as converged with the steps taken so far;
  //  - move the end of the bracket that has the same sign as f(x) up to x;
  //  - try a Newton step: next = x - f(x) / df(x), only if |df(x)| >= 1e-14; accept it only if it lies strictly inside the
  //    bracket (a, b) AND |f(next)| is not more than half of |f(x)|;
  //  - otherwise take the middle of the bracket.
  // After max_iterations report x with converged = |f(x)| <= tol.
  (void)f;
  (void)df;
  (void)a;
  (void)b;
  (void)tol;
  (void)max_iterations;
  return Root();
}

#endif  // LESSON_SOLUTION_HPP
