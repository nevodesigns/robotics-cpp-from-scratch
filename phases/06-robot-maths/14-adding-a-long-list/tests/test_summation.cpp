#include <rc/test/rc_test.hpp>

#include <cmath>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <vector>

#include <rc/core/random.hpp>

#include "solution.hpp"

namespace {

const int kCount = 4000000;

// Numbers that are whole multiples of 2^-40, so that their exact total can be found with integers: the sum of the
// integers k is below 2^64 for 4 million of them, and the total is that sum times 2^-40, with a single rounding at the end.
struct Data {
  std::vector<double> values;
  double exact = 0.0;
};

Data make_data(unsigned seed, int n) {
  rc::core::Rng rng(seed);
  Data d;
  d.values.reserve(static_cast<std::size_t>(n));
  std::uint64_t total = 0;
  for (int i = 0; i < n; ++i) {
    // A 40 bit integer from two draws.
    const std::uint64_t high = rng.next();   // one draw at a time: the order of the two sides of ^ is the compiler's choice
    const std::uint64_t low = rng.next();
    const std::uint64_t k = (high << 8 ^ low) & ((1ULL << 40) - 1);
    total += k;
    d.values.push_back(std::ldexp(static_cast<double>(k), -40));
  }
  d.exact = std::ldexp(static_cast<double>(total), -40);
  return d;
}

double relative_error(double got, double truth) { return std::fabs(got - truth) / std::fabs(truth); }

}  // namespace

RC_TEST("the three totals agree on a short list, and an empty list is zero") {
  const std::vector<double> v = {1.0, 2.5, -0.5, 4.0};
  RC_CHECK_NEAR(sum_naive(v), 7.0, 1e-15);
  RC_CHECK_NEAR(sum_kahan(v), 7.0, 1e-15);
  RC_CHECK_NEAR(sum_pairwise(v), 7.0, 1e-15);
  RC_CHECK_NEAR(sum_kahan(std::vector<double>()), 0.0, 1e-300);
  RC_CHECK_NEAR(sum_pairwise(std::vector<double>()), 0.0, 1e-300);

  // A list long enough to take the pairwise sum through its splitting.
  std::vector<double> ones(1000, 1.0);
  RC_CHECK_NEAR(sum_pairwise(ones), 1000.0, 1e-12);
  RC_CHECK_NEAR(sum_kahan(ones), 1000.0, 1e-12);
}

RC_TEST("a thousand ones next to a number ten to the sixteen vanish from the plain total, Kahan keeps them, and pairwise keeps most") {
  std::vector<double> v;
  v.push_back(1e16);
  for (int i = 0; i < 1000; ++i) v.push_back(1.0);
  v.push_back(-1e16);
  const double naive = sum_naive(v);
  const double kahan = sum_kahan(v);
  const double pairwise = sum_pairwise(v);
  std::cout << "  1e16, then a thousand ones, then -1e16: the exact total is 1000; plain " << std::fixed << std::setprecision(1) << naive
            << ", Kahan " << kahan << ", pairwise " << pairwise << "\n";
  RC_CHECK_NEAR(naive, 0.0, 1e-9);           // 1e16 + 1 is 1e16 in double: every one is lost
  RC_CHECK_NEAR(kahan, 1000.0, 1e-9);
  RC_CHECK(pairwise > 900.0);                // pairwise is not magic: it lost two of them, but it did not lose them all
  RC_CHECK(pairwise < 1000.0);
}

RC_TEST("four million numbers: the plain total is wrong in the fourteenth digit, and the other two are right to the last one") {
  const Data d = make_data(2026, kCount);
  const double naive = sum_naive(d.values);
  const double kahan = sum_kahan(d.values);
  const double pairwise = sum_pairwise(d.values);
  const double e_naive = relative_error(naive, d.exact);
  const double e_kahan = relative_error(kahan, d.exact);
  const double e_pairwise = relative_error(pairwise, d.exact);
  std::cout << "\n  " << kCount << " numbers, an exact total of " << std::setprecision(6) << d.exact << ", relative error:\n"
            << "    plain     " << std::scientific << std::setprecision(2) << e_naive << "\n"
            << "    pairwise  " << e_pairwise << "\n"
            << "    Kahan     " << e_kahan << "\n\n";
  RC_CHECK(e_naive > 1e-14);
  RC_CHECK(e_pairwise < 5e-16);
  RC_CHECK(e_pairwise < e_naive / 10.0);
  RC_CHECK(e_kahan < 5e-16);
  RC_CHECK(e_kahan < e_naive / 10.0);
}

RC_TEST("a million tenths in single precision: the plain total is a percent out, and Kahan's is right to the digits a float has") {
  std::vector<float> v(1000000, 0.1f);
  const float naive = sum_naive_float(v);
  const float kahan = sum_kahan_float(v);
  // The exact total of a million copies of the float 0.1f is 100000.001490116...
  const double truth = 1000000.0 * static_cast<double>(0.1f);
  std::cout << "  a million copies of 0.1 as a float: the exact total is " << std::fixed << std::setprecision(2) << truth << ", plain "
            << naive << ", Kahan " << kahan << "\n\n";
  RC_CHECK(std::fabs(static_cast<double>(naive) - truth) > 500.0);
  RC_CHECK(std::fabs(static_cast<double>(kahan) - truth) < 0.1);
}
