#include <rc/test/rc_test.hpp>

#include <cmath>
#include <iomanip>
#include <iostream>
#include <string>

#include "solution.hpp"

namespace {

// The classic: where does cos(x) equal x? About 0.739085133215161.
const double kDottie = 0.7390851332151607;

Function f_cos() { return [](double x) { return std::cos(x) - x; }; }
Function d_cos() { return [](double x) { return -std::sin(x) - 1.0; }; }

// A cube root: the root is at zero, and the slope there is infinite.
Function f_cbrt() { return [](double x) { return std::cbrt(x); }; }
Function d_cbrt() { return [](double x) { return std::cbrt(x) / (3.0 * x); }; }

// An arctangent, which flattens out far from zero.
Function f_atan() { return [](double x) { return std::atan(x); }; }
Function d_atan() { return [](double x) { return 1.0 / (1.0 + x * x); }; }

}  // namespace

RC_TEST("halving finds the crossing, in about forty steps for an interval of one and a tolerance of 1e-12") {
  const Root r = bisect(f_cos(), 0.0, 1.0, 1e-12);
  std::cout << "  bisection on cos(x) - x over [0, 1]: x = " << std::setprecision(15) << r.x << " after " << r.iterations << " steps\n";
  RC_CHECK(r.converged);
  RC_CHECK_NEAR(r.x, kDottie, 1e-11);
  RC_CHECK_EQ(r.iterations, 40);

  // An end that is exactly the root, and the same sign at both ends.
  const Function line = [](double x) { return x - 3.0; };
  RC_CHECK(bisect(line, 3.0, 5.0, 1e-9).converged);
  RC_CHECK_NEAR(bisect(line, 3.0, 5.0, 1e-9).x, 3.0, 1e-12);
  RC_CHECK(!bisect(line, 4.0, 5.0, 1e-9).converged);        // both ends above zero: nothing says there is a crossing
  const Root wider = bisect(line, -10.0, 100.0, 1e-9);
  RC_CHECK(wider.converged);
  RC_CHECK_NEAR(wider.x, 3.0, 1e-8);
}

RC_TEST("Newton from a good guess takes a handful of steps, as the number of correct digits roughly doubles") {
  const Root r = newton(f_cos(), d_cos(), 1.0, 1e-12, 50);
  std::cout << "  Newton on cos(x) - x from 1.0: x = " << std::setprecision(15) << r.x << " after " << r.iterations << " steps; errors";
  double x = 1.0;
  for (int i = 0; i < 5; ++i) {
    std::cout << " " << std::scientific << std::setprecision(1) << std::fabs(x - kDottie);
    x -= f_cos()(x) / d_cos()(x);
  }
  std::cout << "\n\n";
  RC_CHECK(r.converged);
  RC_CHECK_NEAR(r.x, kDottie, 1e-11);
  RC_CHECK(r.iterations <= 6);
  RC_CHECK(r.iterations >= 3);
}

RC_TEST("Newton can run away from the root, and says so") {
  // The cube root: each step doubles the distance from zero.
  const Root cube = newton(f_cbrt(), d_cbrt(), 1.0, 1e-12, 100);
  // The arctangent from 2: the tangent is so flat that the next guess is further out, and so on.
  const Root arc = newton(f_atan(), d_atan(), 2.0, 1e-12, 100);
  // A flat start: the slope of x^2 - 2 at zero is zero.
  const Root flat = newton([](double x) { return x * x - 2.0; }, [](double x) { return 2.0 * x; }, 0.0, 1e-12, 100);
  std::cout << "  cube root from 1: " << (cube.converged ? "converged" : "failed") << " after " << cube.iterations
            << " steps at x = " << std::scientific << std::setprecision(2) << cube.x << "\n"
            << "  arctangent from 2: " << (arc.converged ? "converged" : "failed") << " after " << arc.iterations << " steps at x = "
            << arc.x << "\n"
            << "  x^2 - 2 from 0: " << (flat.converged ? "converged" : "failed") << " (the slope is zero)\n";
  RC_CHECK(!cube.converged);
  RC_CHECK(!arc.converged);
  RC_CHECK(!flat.converged);
  RC_CHECK(std::fabs(cube.x) > 1.0);
}

RC_TEST("Newton with a bracket takes Newton's steps while they help, and always gets there") {
  const Root cos_root = safeguarded(f_cos(), d_cos(), 0.0, 1.0, 1e-12, 100);
  const Root cube = safeguarded(f_cbrt(), d_cbrt(), -1.0, 2.0, 1e-9, 200);
  const Root arc = safeguarded(f_atan(), d_atan(), -3.3, 5.1, 1e-12, 100);
  const Root bis_cos = bisect(f_cos(), 0.0, 1.0, 1e-12);
  const Root bis_arc = bisect(f_atan(), -3.3, 5.1, 1e-12);

  std::cout << "\n  function         bisection   Newton alone   with a bracket\n";
  std::cout << "  cos(x) - x       " << std::setw(9) << bis_cos.iterations << "   " << std::setw(12) << newton(f_cos(), d_cos(), 1.0, 1e-12, 50).iterations
            << "   " << std::setw(14) << cos_root.iterations << "\n";
  std::cout << "  arctangent       " << std::setw(9) << bis_arc.iterations << "   " << std::setw(12) << "fails" << "   " << std::setw(14)
            << arc.iterations << "\n";
  std::cout << "  cube root        " << std::setw(9) << bisect(f_cbrt(), -1.0, 2.0, 1e-9).iterations << "   " << std::setw(12) << "fails"
            << "   " << std::setw(14) << cube.iterations << "\n\n";

  RC_CHECK(cos_root.converged);
  RC_CHECK_NEAR(cos_root.x, kDottie, 1e-10);
  RC_CHECK(arc.converged);
  RC_CHECK_NEAR(arc.x, 0.0, 1e-9);
  RC_CHECK(cube.converged);
  RC_CHECK_NEAR(cube.x, 0.0, 1e-8);
  RC_CHECK(cos_root.iterations < bis_cos.iterations / 3);        // where Newton works it is nearly as fast as Newton
  RC_CHECK(!safeguarded(f_cos(), d_cos(), 2.0, 3.0, 1e-12, 100).converged);    // no sign change, so no promise
}
