#include <rc/test/rc_test.hpp>

#include <vector>

#include "solution.hpp"

RC_TEST("the mean of an empty span is zero") {
  RC_CHECK_NEAR(mean({}), 0.0, 1e-9);
}

RC_TEST("the mean of a plain array") {
  const double values[] = {1.0, 2.0, 3.0, 4.0};
  RC_CHECK_NEAR(mean(values), 2.5, 1e-9);
}

RC_TEST("the mean of a vector, with no copying required") {
  const std::vector<double> values{2.0, 4.0, 9.0};
  RC_CHECK_NEAR(mean(values), 5.0, 1e-9);
}

RC_TEST("the median of an odd count is the middle value") {
  const double values[] = {5.0, 1.0, 3.0};
  RC_CHECK_NEAR(median(values), 3.0, 1e-9);
}

RC_TEST("the median of an even count averages the middle pair") {
  const double values[] = {4.0, 1.0, 3.0, 2.0};
  RC_CHECK_NEAR(median(values), 2.5, 1e-9);
}

RC_TEST("the median ignores a single wild reading, the mean does not") {
  const double values[] = {1.00, 1.02, 0.98, 1.01, 4.70};
  RC_CHECK_NEAR(median(values), 1.01, 1e-9);
  RC_CHECK(mean(values) > 1.5);
}

RC_TEST("the median does not reorder the caller's data") {
  std::vector<double> values{5.0, 1.0, 3.0};
  median(values);
  RC_REQUIRE_EQ(values.size(), std::size_t{3});
  RC_CHECK_NEAR(values[0], 5.0, 1e-9);
  RC_CHECK_NEAR(values[1], 1.0, 1e-9);
}

RC_TEST("a moving average returns one value per reading") {
  const double values[] = {1.0, 2.0, 3.0, 4.0};
  const std::vector<double> smoothed = moving_average(values, 2);
  RC_CHECK_EQ(smoothed.size(), std::size_t{4});
}

RC_TEST("the first moving average value is the first reading") {
  const double values[] = {1.0, 2.0, 3.0, 4.0};
  const std::vector<double> smoothed = moving_average(values, 3);
  RC_REQUIRE_EQ(smoothed.size(), std::size_t{4});
  RC_CHECK_NEAR(smoothed[0], 1.0, 1e-9);
  RC_CHECK_NEAR(smoothed[1], 1.5, 1e-9);
  RC_CHECK_NEAR(smoothed[2], 2.0, 1e-9);
  RC_CHECK_NEAR(smoothed[3], 3.0, 1e-9);
}

RC_TEST("a window below one is refused rather than guessed at") {
  const double values[] = {1.0, 2.0};
  RC_CHECK(moving_average(values, 0).empty());
  RC_CHECK(moving_average(values, -5).empty());
}
