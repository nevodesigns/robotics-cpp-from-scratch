#include <rc/test/rc_test.hpp>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <limits>
#include <utility>
#include <vector>

#include "solution.hpp"

namespace {

struct Rng {
  std::uint32_t state;
  std::uint32_t next() {
    state ^= state << 13;
    state ^= state >> 17;
    state ^= state << 5;
    return state;
  }
  double uniform() { return static_cast<double>(next() >> 8) / 16777216.0; }
};

// The same elimination with the pivoting left out: the baseline that shows what it is for.
std::vector<double> solve_without_pivoting(Matrix a, std::vector<double> b) {
  const std::size_t n = b.size();
  for (std::size_t k = 0; k < n; ++k) {
    for (std::size_t i = k + 1; i < n; ++i) {
      const double factor = a.at(i, k) / a.at(k, k);
      for (std::size_t j = k; j < n; ++j) a.at(i, j) -= factor * a.at(k, j);
      b[i] -= factor * b[k];
    }
  }
  std::vector<double> x(n, 0.0);
  for (std::size_t i = n; i-- > 0;) {
    double s = b[i];
    for (std::size_t j = i + 1; j < n; ++j) s -= a.at(i, j) * x[j];
    x[i] = s / a.at(i, i);
  }
  return x;
}

Matrix hilbert(std::size_t n) {
  Matrix h(n, n);
  for (std::size_t i = 0; i < n; ++i)
    for (std::size_t j = 0; j < n; ++j) h.at(i, j) = 1.0 / static_cast<double>(i + j + 1);
  return h;
}

double largest_error(const std::vector<double>& x, const std::vector<double>& truth) {
  // An answer of the wrong length is infinitely wrong, and is not indexed: an empty
  // implementation returns an empty answer, and reading it would crash the test instead
  // of failing it.
  if (x.size() != truth.size()) return std::numeric_limits<double>::infinity();
  double e = 0.0;
  for (std::size_t i = 0; i < truth.size(); ++i) e = std::max(e, std::fabs(x[i] - truth[i]));
  return e;
}

// A polynomial fit to a hundred points on 0 to 1, with a known answer and no noise: the
// coefficient of t^j is 1 / (j + 1). Whatever is wrong with the answer is the method.
struct Fit {
  Matrix design;
  std::vector<double> samples;
  std::vector<double> truth;
};

Fit polynomial_fit(std::size_t degree) {
  const std::size_t points = 100;
  const std::size_t columns = degree + 1;
  Fit fit{Matrix(points, columns), std::vector<double>(points, 0.0), std::vector<double>(columns, 0.0)};
  for (std::size_t j = 0; j < columns; ++j) fit.truth[j] = 1.0 / static_cast<double>(j + 1);

  for (std::size_t i = 0; i < points; ++i) {
    const double t = static_cast<double>(i) / static_cast<double>(points - 1);
    double power = 1.0;
    for (std::size_t j = 0; j < columns; ++j) {
      fit.design.at(i, j) = power;
      fit.samples[i] += fit.truth[j] * power;
      power *= t;
    }
  }
  return fit;
}

}  // namespace

RC_TEST("a system with a known answer is solved, whatever order its rows come in") {
  Rng rng{3};
  for (const std::size_t n : {2u, 3u, 5u, 8u, 12u}) {
    Matrix a(n, n);
    std::vector<double> truth(n);
    for (std::size_t i = 0; i < n; ++i) {
      truth[i] = 2.0 * rng.uniform() - 1.0;
      for (std::size_t j = 0; j < n; ++j) a.at(i, j) = 2.0 * rng.uniform() - 1.0;
      a.at(i, i) += 3.0;   // comfortably invertible
    }
    const std::vector<double> b = multiply(a, truth);

    bool ok = false;
    const std::vector<double> x = solve(a, b, ok);
    RC_REQUIRE(ok);
    RC_CHECK(largest_error(x, truth) < 1e-12);
  }

  // A zero in the pivot position, which elimination without a swap divides by.
  Matrix swapped(2, 2);
  swapped.at(0, 0) = 0.0;
  swapped.at(0, 1) = 1.0;
  swapped.at(1, 0) = 1.0;
  swapped.at(1, 1) = 0.0;
  bool ok = false;
  const std::vector<double> x = solve(swapped, {3.0, 5.0}, ok);
  RC_REQUIRE(ok);
  RC_CHECK_NEAR(x[0], 5.0, 1e-12);
  RC_CHECK_NEAR(x[1], 3.0, 1e-12);
}

RC_TEST("a tiny pivot ruins an elimination that does not swap, and costs a swap nothing") {
  Matrix a(2, 2);
  a.at(0, 0) = 1e-20;
  a.at(0, 1) = 1.0;
  a.at(1, 0) = 1.0;
  a.at(1, 1) = 1.0;
  const std::vector<double> b = {1.0, 2.0};   // the answer is x = 1, y = 1, to a part in 10^20

  const std::vector<double> bad = solve_without_pivoting(a, b);
  bool ok = false;
  const std::vector<double> good = solve(a, b, ok);
  RC_REQUIRE(ok);

  std::cout << "  the matrix [[1e-20, 1], [1, 1]] and the right hand side [1, 2], whose answer is (1, 1):\n"
            << "    elimination without swapping rows   (" << std::setprecision(3) << bad[0] << ", " << bad[1] << ")\n"
            << "    with the largest entry as the pivot (" << good[0] << ", " << good[1] << ")\n\n";

  // The multiple is 10^20, the second row is swamped by it, and x comes out as zero.
  RC_CHECK(std::fabs(bad[0] - 1.0) > 0.5);
  RC_CHECK_NEAR(good[0], 1.0, 1e-12);
  RC_CHECK_NEAR(good[1], 1.0, 1e-12);
}

RC_TEST("a singular matrix is reported, and a nearly singular one is solved") {
  Matrix singular(2, 2);
  singular.at(0, 0) = 1.0;
  singular.at(0, 1) = 2.0;
  singular.at(1, 0) = 2.0;
  singular.at(1, 1) = 4.0;
  bool ok = true;
  solve(singular, {1.0, 2.0}, ok);
  RC_CHECK(!ok);
  RC_CHECK(std::isinf(condition_1norm(singular)));

  Matrix nearly(2, 2);
  nearly.at(0, 0) = 1.0;
  nearly.at(0, 1) = 2.0;
  nearly.at(1, 0) = 2.0;
  nearly.at(1, 1) = 4.000001;
  const std::vector<double> x = solve(nearly, {1.0, 2.0}, ok);
  RC_CHECK(ok);          // it is solved, and ok says nothing about whether the answer can be trusted
  RC_CHECK(x.size() == 2);
  RC_CHECK(condition_1norm(nearly) > 1e6);

  // Not square, not matched, and empty are refused and not divided.
  bool shape_ok = true;
  solve(Matrix(2, 3), {1.0, 2.0}, shape_ok);
  RC_CHECK(!shape_ok);
  solve(Matrix(2, 2), {1.0, 2.0, 3.0}, shape_ok);
  RC_CHECK(!shape_ok);
  solve(Matrix(0, 0), {}, shape_ok);
  RC_CHECK(!shape_ok);
}

RC_TEST("the condition number is the factor by which a matrix magnifies relative error") {
  Matrix identity(3, 3);
  for (std::size_t i = 0; i < 3; ++i) identity.at(i, i) = 1.0;
  RC_CHECK_NEAR(condition_1norm(identity), 1.0, 1e-12);

  Matrix stretched(2, 2);
  stretched.at(0, 0) = 1.0;
  stretched.at(1, 1) = 1e-6;
  RC_CHECK_NEAR(condition_1norm(stretched), 1e6, 1e-3);

  // The fourth Hilbert matrix has a published 1-norm condition number of 28375.
  RC_CHECK_NEAR(condition_1norm(hilbert(4)), 28375.0, 1.0);

  Matrix rectangular(2, 3);
  RC_CHECK(std::isinf(condition_1norm(rectangular)));
}

RC_TEST("a solver loses as many digits as the condition number says, and no pivoting can help") {
  std::cout << "  the Hilbert matrix, whose entries are 1 / (i + j + 1), solved against a right hand side made\n"
            << "  from the answer (1, 1, ..., 1), so the exact answer is known:\n\n"
            << "    n    condition number    largest error in the answer   digits lost\n";

  double error_at_10 = 0.0;
  double cond_at_12 = 0.0;
  for (const std::size_t n : {4u, 6u, 8u, 10u, 12u}) {
    const Matrix h = hilbert(n);
    const std::vector<double> truth(n, 1.0);
    const std::vector<double> b = multiply(h, truth);

    bool ok = false;
    const std::vector<double> x = solve(h, b, ok);
    const double cond = condition_1norm(h);

    std::cout << "    " << std::setw(2) << n << std::scientific << std::setprecision(2) << std::setw(18) << cond;
    if (!ok) {
      // At this size a pivot falls below the rounding of the largest entry, and the solver
      // declines instead of returning a number.
      std::cout << "      gave up: singular to working precision\n";
      if (n == 12) cond_at_12 = cond;
      continue;
    }

    const double error = largest_error(x, truth);
    std::cout << std::setw(26) << error << std::fixed << std::setprecision(1) << std::setw(14)
              << std::log10(std::fmax(error, 1e-16) / 1e-16) << "\n";

    // The rule of thumb: the error is about the condition number times the rounding of a double.
    RC_CHECK(error < cond * 1e-15);
    if (n == 10) error_at_10 = error;
  }
  std::cout << "\n  Each step of two in n costs about three more digits, and by n = 10 the answer is right to four.\n"
            << "  At n = 12 a pivot falls below the rounding of the largest entry: as far as a double can tell the\n"
            << "  matrix is singular, and the solver says so instead of returning a number. Its true condition\n"
            << "  number is about 10^16, which is all the digits a double has.\n\n";

  RC_CHECK(error_at_10 > 1e-5);
  RC_CHECK(std::isinf(cond_at_12));   // the solver declined, so there is no number to report
}

RC_TEST("fitting a polynomial: squaring the problem costs twice the digits") {
  std::cout << "  a polynomial fitted to a hundred exact samples on 0 to 1, whose coefficients are known. The largest\n"
            << "  error in a coefficient, from the normal equations and from QR:\n\n"
            << "    degree   normal equations    QR\n";

  double normal[4];
  double qr[4];
  const std::size_t degrees[] = {3, 6, 9, 12};
  for (int i = 0; i < 4; ++i) {
    const Fit fit = polynomial_fit(degrees[i]);
    bool ok_normal = false;
    bool ok_qr = false;
    normal[i] = largest_error(least_squares_normal(fit.design, fit.samples, ok_normal), fit.truth);
    qr[i] = largest_error(least_squares_qr(fit.design, fit.samples, ok_qr), fit.truth);
    RC_REQUIRE(ok_qr);
    std::cout << "    " << std::setw(6) << degrees[i] << std::scientific << std::setprecision(2) << std::setw(17)
              << normal[i] << std::setw(12) << qr[i] << "\n";
  }
  std::cout << "\n";

  // QR is better at every degree, and by more as the problem gets harder.
  for (int i = 0; i < 4; ++i) RC_CHECK(qr[i] < normal[i]);
  RC_CHECK(qr[0] < 1e-12);
  RC_CHECK(qr[2] < 1e-8);
  RC_CHECK(qr[3] < 1e-5);

  // The normal equations are fine until they are not, and at degree twelve there is nothing left.
  RC_CHECK(normal[0] < 1e-9);
  RC_CHECK(normal[3] > 0.1);
  RC_CHECK(normal[2] / qr[2] > 1e3);
}

RC_TEST("forming A transpose A squares the condition number, which is where the digits go") {
  const Matrix a = hilbert(6);
  const double cond_a = condition_1norm(a);
  const double cond_ata = condition_1norm(multiply(a.transpose(), a));

  std::cout << "  the sixth Hilbert matrix has a condition number of " << std::scientific << std::setprecision(2) << cond_a
            << ", and the matrix of its normal\n  equations has " << cond_ata << ", which is " << std::fixed
            << std::setprecision(2) << cond_ata / (cond_a * cond_a) << " of the square.\n\n";

  RC_CHECK(cond_ata > 0.1 * cond_a * cond_a);
  RC_CHECK(cond_ata < 10.0 * cond_a * cond_a);

  // And for a matrix that is fine, least squares is the solve: the same answer both ways.
  Matrix well(3, 3);
  well.at(0, 0) = 4.0;
  well.at(0, 1) = 1.0;
  well.at(1, 0) = 1.0;
  well.at(1, 1) = 3.0;
  well.at(1, 2) = 1.0;
  well.at(2, 1) = 1.0;
  well.at(2, 2) = 5.0;
  const std::vector<double> truth = {1.0, -2.0, 3.0};
  const std::vector<double> b = multiply(well, truth);
  bool ok = false;
  const std::vector<double> exact = solve(well, b, ok);
  RC_REQUIRE(ok);
  const std::vector<double> by_qr = least_squares_qr(well, b, ok);
  RC_REQUIRE(ok);
  RC_CHECK(largest_error(exact, truth) < 1e-12);
  RC_CHECK(largest_error(by_qr, truth) < 1e-12);

  // Fewer rows than columns, or dependent columns, have no unique answer.
  bool shape_ok = true;
  least_squares_qr(Matrix(2, 3), {1.0, 2.0}, shape_ok);
  RC_CHECK(!shape_ok);
  Matrix dependent(4, 2);
  for (std::size_t i = 0; i < 4; ++i) {
    dependent.at(i, 0) = static_cast<double>(i + 1);
    dependent.at(i, 1) = 2.0 * static_cast<double>(i + 1);
  }
  least_squares_qr(dependent, {1.0, 2.0, 3.0, 4.0}, shape_ok);
  RC_CHECK(!shape_ok);
}
