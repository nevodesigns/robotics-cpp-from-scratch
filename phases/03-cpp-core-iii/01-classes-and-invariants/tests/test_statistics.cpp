#include <rc/test/rc_test.hpp>

#include <cmath>

#include "solution.hpp"

namespace {

// Takes the object by const reference on purpose. This will not compile unless
// every reading function is marked const, which is the point of the exercise.
double mean_of(const RunningStatistics& statistics) { return statistics.mean(); }

}  // namespace

RC_TEST("a fresh object answers zero to everything") {
  const RunningStatistics statistics;
  RC_CHECK_EQ(statistics.count(), 0);
  RC_CHECK_NEAR(statistics.mean(), 0.0, 1e-12);
  RC_CHECK_NEAR(statistics.variance(), 0.0, 1e-12);
  RC_CHECK_NEAR(statistics.lowest(), 0.0, 1e-12);
}

RC_TEST("one reading sets the mean and both ends") {
  RunningStatistics statistics;
  statistics.add(4.5);
  RC_CHECK_EQ(statistics.count(), 1);
  RC_CHECK_NEAR(statistics.mean(), 4.5, 1e-12);
  RC_CHECK_NEAR(statistics.lowest(), 4.5, 1e-12);
  RC_CHECK_NEAR(statistics.highest(), 4.5, 1e-12);
  RC_CHECK_NEAR(statistics.variance(), 0.0, 1e-12);
}

RC_TEST("the range is seeded from the data, not from zero") {
  // A sensor reading around 900 must not report a lowest of 0.0.
  RunningStatistics statistics;
  statistics.add(900.0);
  statistics.add(910.0);
  RC_CHECK_NEAR(statistics.lowest(), 900.0, 1e-12);
}

RC_TEST("the mean is correct over several readings") {
  RunningStatistics statistics;
  for (const double value : {2.0, 4.0, 4.0, 4.0, 5.0, 5.0, 7.0, 9.0}) statistics.add(value);
  RC_CHECK_EQ(statistics.count(), 8);
  RC_CHECK_NEAR(statistics.mean(), 5.0, 1e-12);
}

RC_TEST("the sample variance matches the textbook value") {
  // For 2 4 4 4 5 5 7 9 the population variance is 4 and the sample variance,
  // dividing by count minus one, is 32/7.
  RunningStatistics statistics;
  for (const double value : {2.0, 4.0, 4.0, 4.0, 5.0, 5.0, 7.0, 9.0}) statistics.add(value);
  RC_CHECK_NEAR(statistics.variance(), 32.0 / 7.0, 1e-9);
  RC_CHECK_NEAR(statistics.standard_deviation(), std::sqrt(32.0 / 7.0), 1e-9);
}

RC_TEST("the lowest and highest bracket the mean") {
  RunningStatistics statistics;
  for (const double value : {3.0, -2.0, 11.5, 0.25}) statistics.add(value);
  RC_CHECK_NEAR(statistics.lowest(), -2.0, 1e-12);
  RC_CHECK_NEAR(statistics.highest(), 11.5, 1e-12);
  RC_CHECK(statistics.mean() >= statistics.lowest());
  RC_CHECK(statistics.mean() <= statistics.highest());
}

RC_TEST("precision survives readings around one million") {
  // This is the test the naive running total fails. The readings differ by a
  // hundredth on top of a large constant, and a sum would spend all its
  // significant digits on the constant part.
  RunningStatistics statistics;
  for (int i = 0; i < 10000; ++i) {
    statistics.add(1000000.0 + (i % 2 == 0 ? 0.01 : -0.01));
  }
  RC_CHECK_NEAR(statistics.mean(), 1000000.0, 1e-6);

  // Every reading sits exactly 0.01 from the mean, so the squared deviations
  // total 10000 * 0.0001 = 1.0. This is the sample variance, which divides by
  // count minus one, so the answer is 1/9999 rather than the population value
  // of 0.0001. The difference is small here and is not a rounding error.
  RC_CHECK(statistics.variance() > 0.0);
  RC_CHECK_NEAR(statistics.variance(), 1.0 / 9999.0, 1e-12);
}

RC_TEST("variance is never negative") {
  // A negative variance is impossible, and it is the reliable signal that a
  // running total has lost the small variations.
  RunningStatistics statistics;
  for (int i = 0; i < 1000; ++i) statistics.add(1e9 + (i % 3));
  RC_CHECK(statistics.variance() >= 0.0);
}

RC_TEST("the reading functions can be called on a const object") {
  // If this does not compile, a reading function is missing its const.
  RunningStatistics statistics;
  statistics.add(2.0);
  statistics.add(4.0);
  RC_CHECK_NEAR(mean_of(statistics), 3.0, 1e-12);
}

RC_TEST("reset returns the object to its starting state") {
  RunningStatistics statistics;
  for (const double value : {5.0, 10.0, 15.0}) statistics.add(value);
  statistics.reset();
  RC_CHECK_EQ(statistics.count(), 0);
  RC_CHECK_NEAR(statistics.mean(), 0.0, 1e-12);
  RC_CHECK_NEAR(statistics.variance(), 0.0, 1e-12);

  statistics.add(7.0);
  RC_CHECK_NEAR(statistics.lowest(), 7.0, 1e-12);
}

RC_TEST("order does not change the answer") {
  RunningStatistics forward;
  for (const double value : {1.0, 2.0, 3.0, 4.0, 5.0}) forward.add(value);

  RunningStatistics backward;
  for (const double value : {5.0, 4.0, 3.0, 2.0, 1.0}) backward.add(value);

  RC_CHECK_NEAR(forward.mean(), backward.mean(), 1e-12);
  RC_CHECK_NEAR(forward.variance(), backward.variance(), 1e-12);
}
