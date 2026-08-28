#include <rc/test/rc_test.hpp>
#include <rc/test/leak_check.hpp>

#include "solution.hpp"

namespace {

using rc::test::LeakCheck;

const double kValues[] = {1.0, 2.0, 3.0, 4.0, 5.0};

}  // namespace

RC_TEST("the average is correct") {
  RC_CHECK_NEAR(average_of_readings(kValues, 5), 3.0, 1e-9);
}

RC_TEST("the average of nothing is zero, and leaks nothing") {
  const LeakCheck check;
  RC_CHECK_NEAR(average_of_readings(kValues, 0), 0.0, 1e-9);
  RC_CHECK_NEAR(average_of_readings(kValues, -2), 0.0, 1e-9);
  RC_CHECK(check.balanced());
}

RC_TEST("the average leaks nothing on the ordinary path") {
  const LeakCheck check;
  average_of_readings(kValues, 5);
  RC_CHECK(check.balanced());
}

RC_TEST("counting above a limit is correct") {
  RC_CHECK_EQ(count_above(kValues, 5, 2.5, 100), 3);
  RC_CHECK_EQ(count_above(kValues, 5, 9.0, 100), 0);
}

RC_TEST("giving up early still reports what it found") {
  // Three values are above 2.5, and we give up after finding more than one.
  RC_CHECK_EQ(count_above(kValues, 5, 2.5, 1), 2);
}

RC_TEST("giving up early leaks nothing") {
  const LeakCheck check;
  count_above(kValues, 5, 2.5, 1);
  RC_CHECK(check.balanced());
}

RC_TEST("finding the first valid sample is correct") {
  const Sample samples[] = {{1.0, false}, {2.0, false}, {3.0, true}, {4.0, true}};
  const Sample found = first_valid_or_default(samples, 4);
  RC_CHECK_NEAR(found.value, 3.0, 1e-9);
  RC_CHECK(found.valid);
}

RC_TEST("no valid sample gives a default, and leaks nothing") {
  const LeakCheck check;
  const Sample none[] = {{1.0, false}, {2.0, false}};
  const Sample found = first_valid_or_default(none, 2);
  RC_CHECK_NEAR(found.value, 0.0, 1e-9);
  RC_CHECK(check.balanced());
}

RC_TEST("the early return path gives back an array as an array") {
  // This is the check that catches new[] paired with plain delete. The counters
  // are kept separately, so a mismatched pair leaves one of them wrong even
  // when the total number of allocations and frees happens to match.
  const LeakCheck check;
  const Sample samples[] = {{1.0, true}, {2.0, true}};
  first_valid_or_default(samples, 2);
  RC_CHECK(check.balanced());
}

RC_TEST("every function survives an empty input without allocating a leak") {
  const LeakCheck check;
  average_of_readings(kValues, 0);
  count_above(kValues, 0, 1.0, 1);
  first_valid_or_default(nullptr, 0);
  RC_CHECK(check.balanced());
}
