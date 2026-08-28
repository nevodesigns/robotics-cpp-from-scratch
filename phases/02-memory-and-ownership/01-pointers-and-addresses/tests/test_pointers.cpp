#include <rc/test/rc_test.hpp>

#include "solution.hpp"

namespace {
const Reading kSamples[] = {
    {20.0, 0}, {45.5, 100}, {81.25, 200}, {79.0, 300}, {90.5, 400},
};
constexpr int kCount = 5;
}  // namespace

RC_TEST("finds the first reading above the limit") {
  const Reading* found = find_first_above(kSamples, kCount, 80.0);
  RC_REQUIRE(found != nullptr);
  RC_CHECK_NEAR(found->celsius, 81.25, 1e-9);
  RC_CHECK_EQ(found->milliseconds, 200);
}

RC_TEST("answers nothing when no reading qualifies") {
  RC_CHECK(find_first_above(kSamples, kCount, 200.0) == nullptr);
}

RC_TEST("answers nothing rather than reading from nowhere") {
  RC_CHECK(find_first_above(nullptr, 5, 0.0) == nullptr);
  RC_CHECK(find_first_above(kSamples, 0, 0.0) == nullptr);
  RC_CHECK(find_first_above(kSamples, -3, 0.0) == nullptr);
}

RC_TEST("the answer points into the caller's array, not at a copy") {
  const Reading* found = find_first_above(kSamples, kCount, 80.0);
  RC_REQUIRE(found != nullptr);
  // The address returned must be the address of element two, not of some
  // temporary that happens to hold the same value.
  RC_CHECK(found == &kSamples[2]);
}

RC_TEST("swapping through pointers changes the caller's readings") {
  Reading first{1.0, 10};
  Reading second{2.0, 20};
  swap_readings(&first, &second);
  RC_CHECK_NEAR(first.celsius, 2.0, 1e-9);
  RC_CHECK_EQ(first.milliseconds, 20);
  RC_CHECK_NEAR(second.celsius, 1.0, 1e-9);
}

RC_TEST("swapping with a null pointer does nothing and does not crash") {
  Reading only{7.0, 70};
  swap_readings(&only, nullptr);
  swap_readings(nullptr, &only);
  swap_readings(nullptr, nullptr);
  RC_CHECK_NEAR(only.celsius, 7.0, 1e-9);
}

RC_TEST("finds the highest reading") {
  const Reading* peak = highest(kSamples, kCount);
  RC_REQUIRE(peak != nullptr);
  RC_CHECK_NEAR(peak->celsius, 90.5, 1e-9);
  RC_CHECK(peak == &kSamples[4]);
}

RC_TEST("the highest of nothing is nothing") {
  RC_CHECK(highest(nullptr, 3) == nullptr);
  RC_CHECK(highest(kSamples, 0) == nullptr);
}

RC_TEST("a single reading is its own highest") {
  const Reading one[] = {{5.0, 1}};
  const Reading* peak = highest(one, 1);
  RC_REQUIRE(peak != nullptr);
  RC_CHECK(peak == &one[0]);
}
