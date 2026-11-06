#include <rc/test/rc_test.hpp>

#include <cmath>
#include <iomanip>
#include <iostream>

#include "solution.hpp"

static_assert(sizeof(unsigned int) == 4, "the generator assumes a 32 bit unsigned int");

RC_TEST("the same seed gives the same numbers and another seed does not") {
  Rng a(7);
  Rng b(7);
  bool same = true;
  for (int i = 0; i < 1000; ++i) {
    if (a.next() != b.next()) same = false;
  }
  RC_CHECK(same);

  Rng c(1);
  Rng d(2);
  int equal = 0;
  for (int i = 0; i < 10; ++i) {
    if (c.next() == d.next()) ++equal;
  }
  RC_CHECK_EQ(equal, 0);

  // The first numbers of xorshift32 from seed 1, worked out separately.
  Rng known(1);
  RC_CHECK_EQ(known.next(), 270369u);
  RC_CHECK_EQ(known.next(), 67634689u);
  RC_CHECK_EQ(known.next(), 2647435461u);
  RC_CHECK_EQ(known.next(), 307599695u);
}

RC_TEST("a seed of zero does not leave the generator stuck at zero") {
  Rng zero(0);
  const unsigned int first = zero.next();
  const unsigned int second = zero.next();
  std::cout << "  seed 0: first numbers " << first << " and " << second << "\n\n";
  RC_CHECK(first != 0u);
  RC_CHECK(second != 0u);
  RC_CHECK(first != second);
}

RC_TEST("below(n) stays below n, and each value is equally likely") {
  Rng rng(3);
  const unsigned int n = 6;
  int count[6] = {0, 0, 0, 0, 0, 0};
  const int draws = 600000;
  bool in_range = true;
  for (int i = 0; i < draws; ++i) {
    const unsigned int v = rng.below(n);
    if (v >= n) {
      in_range = false;
      continue;
    }
    ++count[v];
  }
  RC_CHECK(in_range);
  int low = draws;
  int high = 0;
  for (int c : count) {
    if (c < low) low = c;
    if (c > high) high = c;
  }
  std::cout << "  a die, " << draws << " throws: fewest " << low << ", most " << high << " (fair is 100000)\n\n";
  RC_CHECK(low > 98000);
  RC_CHECK(high < 102000);
  RC_CHECK_EQ(rng.below(0), 0u);
  RC_CHECK_EQ(rng.below(1), 0u);
}

RC_TEST("the quick way, a remainder, favours small values, and below(n) does not") {
  // n of 3,000,000,000 is not a divisor of 2^32, so the remainders below 1,294,967,296 can arise
  // twice and the rest only once.
  const unsigned int n = 3000000000u;
  const unsigned int boundary = 1294967296u;
  const int draws = 400000;

  Rng quick(11);
  Rng fair(11);
  int quick_low = 0;
  int fair_low = 0;
  for (int i = 0; i < draws; ++i) {
    if (quick.next() % n < boundary) ++quick_low;
    if (fair.below(n) < boundary) ++fair_low;
  }
  const double expected_fair = static_cast<double>(boundary) / static_cast<double>(n);
  const double quick_share = static_cast<double>(quick_low) / draws;
  const double fair_share = static_cast<double>(fair_low) / draws;
  std::cout << "  share of draws below " << boundary << " out of 3000000000: remainder " << std::fixed
            << std::setprecision(3) << quick_share << ", below(n) " << fair_share << ", fair " << expected_fair << "\n\n";
  RC_CHECK(quick_share > 0.55);
  RC_CHECK_NEAR(fair_share, expected_fair, 0.01);
}

RC_TEST("uniform() lies in zero to one with mean a half and variance a twelfth") {
  Rng rng(5);
  const int draws = 200000;
  double sum = 0.0;
  double sum_squares = 0.0;
  bool in_range = true;
  for (int i = 0; i < draws; ++i) {
    const double u = rng.uniform();
    if (u < 0.0 || u >= 1.0) in_range = false;
    sum += u;
    sum_squares += u * u;
  }
  const double mean = sum / draws;
  const double variance = sum_squares / draws - mean * mean;
  std::cout << "  uniform: mean " << std::fixed << std::setprecision(4) << mean << " (0.5), variance " << variance
            << " (0.0833)\n\n";
  RC_CHECK(in_range);
  RC_CHECK_NEAR(mean, 0.5, 0.005);
  RC_CHECK_NEAR(variance, 1.0 / 12.0, 0.002);
}

RC_TEST("gaussian() has mean zero and spread one, and never strays past six") {
  Rng rng(9);
  const int draws = 400000;
  double sum = 0.0;
  double sum_squares = 0.0;
  int beyond_three = 0;
  double biggest = 0.0;
  for (int i = 0; i < draws; ++i) {
    const double g = rng.gaussian();
    sum += g;
    sum_squares += g * g;
    if (std::fabs(g) > 3.0) ++beyond_three;
    if (std::fabs(g) > biggest) biggest = std::fabs(g);
  }
  const double mean = sum / draws;
  const double sd = std::sqrt(sum_squares / draws - mean * mean);
  const double tail = 100.0 * beyond_three / draws;
  std::cout << "  gaussian: mean " << std::fixed << std::setprecision(4) << mean << ", spread " << sd
            << ", beyond 3 sigma " << std::setprecision(3) << tail << " percent (a true bell curve: 0.270), largest "
            << std::setprecision(2) << biggest << "\n\n";
  RC_CHECK_NEAR(mean, 0.0, 0.01);
  RC_CHECK_NEAR(sd, 1.0, 0.01);
  RC_CHECK(biggest <= 6.0);
  RC_CHECK(tail > 0.1);
  RC_CHECK(tail < 0.4);
}

RC_TEST("a failure found with a seed is found again with it") {
  // Under seed 12345 take the largest of 100 draws, as a stand-in for the input that broke something.
  double largest = -1.0;
  int index = -1;
  {
    Rng rng(12345);
    for (int i = 0; i < 100; ++i) {
      const double u = rng.uniform();
      if (u > largest) {
        largest = u;
        index = i;
      }
    }
  }

  double largest_again = -1.0;
  int index_again = -1;
  {
    Rng rng(12345);
    for (int i = 0; i < 100; ++i) {
      const double u = rng.uniform();
      if (u > largest_again) {
        largest_again = u;
        index_again = i;
      }
    }
  }
  std::cout << "  seed 12345, the largest of 100 draws: " << std::fixed << std::setprecision(6) << largest
            << " at draw " << index << "; run again " << largest_again << " at draw " << index_again << "\n\n";
  RC_CHECK(largest > 0.95);
  RC_CHECK_EQ(index_again, index);
  RC_CHECK(largest_again == largest);
}
