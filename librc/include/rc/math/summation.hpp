// rc/math/summation.hpp
//
// Adding a long list, from lesson 06-14, graduated.
//
// Every addition rounds, and a small number added to a large total loses its low digits. 1e16, then a thousand ones, then -1e16 has the exact total 1000: the plain total was 0.0, because 1e16 + 1 is 1e16 in a double, Kahan's total 1000.0 and
// the pairwise total 998.0, which is not magic. Over 4 million numbers whose exact total is known from integers, the plain total had a relative error of 2.53e-14 and the pairwise and Kahan totals 0. A million copies of 0.1 as a float
// gave 100958.34 plain, 0.96 percent out, and 100000.00 with Kahan, against an exact 100000.0015.

#ifndef RC_MATH_SUMMATION_HPP
#define RC_MATH_SUMMATION_HPP

#include <cmath>
#include <cstddef>
#include <vector>

namespace rc {
namespace math {

// The plain total: add each number to a running sum. Given.
//
// Every addition rounds its answer to the nearest double, and when the running sum is large and the number is small, the
// small number's low digits are lost. A hundred million additions lose a hundred million small pieces.
inline double sum_naive(const std::vector<double>& v) {
  double total = 0.0;
  for (double x : v) total += x;
  return total;
}

// Kahan's sum: keep what each addition lost, and put it back into the next one.
//
// `lost` holds the part of the last number that did not fit into the total. The next number is first corrected by it, then
// added, and the new loss is worked out as (new total - old total) - corrected number, which is exactly the part that was rounded away.
inline double sum_kahan(const std::vector<double>& v) {
  double total = 0.0;
  double lost = 0.0;
  for (double x : v) {
    const double corrected = x - lost;
    const double next = total + corrected;
    lost = (next - total) - corrected;
    total = next;
  }
  return total;
}

// Add from the middle out: split the list in half, total each half, and add the two. A total of a million numbers is then
// built of additions of numbers of the same size, a million small ones first and the big ones only at the end. Short pieces are added plainly.
inline double sum_pairwise_range(const double* data, std::size_t n) {
  if (n <= 8) {
    double total = 0.0;
    for (std::size_t i = 0; i < n; ++i) total += data[i];
    return total;
  }
  const std::size_t half = n / 2;
  return sum_pairwise_range(data, half) + sum_pairwise_range(data + half, n - half);
}

inline double sum_pairwise(const std::vector<double>& v) { return v.empty() ? 0.0 : sum_pairwise_range(v.data(), v.size()); }

// The same two ideas for single precision numbers, where the loss is much faster to see.
inline float sum_naive_float(const std::vector<float>& v) {
  float total = 0.0f;
  for (float x : v) total += x;
  return total;
}

inline float sum_kahan_float(const std::vector<float>& v) {
  float total = 0.0f;
  float lost = 0.0f;
  for (float x : v) {
    const float corrected = x - lost;
    const float next = total + corrected;
    lost = (next - total) - corrected;
    total = next;
  }
  return total;
}

}  // namespace math
}  // namespace rc

#endif  // RC_MATH_SUMMATION_HPP
