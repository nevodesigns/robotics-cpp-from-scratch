#include <rc/test/rc_test.hpp>

#include <vector>

#include "solution.hpp"

namespace {

std::vector<Reading> batch() {
  return {
      {20.0, 300, true},
      {85.5, 100, true},
      {19.0, 200, false},
      {90.0, 400, true},
      {18.0, 500, false},
  };
}

}  // namespace

RC_TEST("the first reading above a limit is found") {
  const auto found = first_above(batch(), 80.0);
  RC_REQUIRE(found.has_value());
  RC_CHECK_NEAR(found->celsius, 85.5, 1e-12);
  RC_CHECK_EQ(found->at_ms, 100L);
}

RC_TEST("nothing above the limit answers nothing") {
  RC_CHECK(!first_above(batch(), 200.0).has_value());
  RC_CHECK(!first_above({}, 0.0).has_value());
}

RC_TEST("the first means the first in order, not the largest") {
  // 85.5 comes before 90.0 in the batch, so it is the answer even though it is
  // the smaller of the two above the limit.
  const auto found = first_above(batch(), 80.0);
  RC_REQUIRE(found.has_value());
  RC_CHECK_NEAR(found->celsius, 85.5, 1e-12);
}

RC_TEST("valid readings are counted") {
  RC_CHECK_EQ(count_valid(batch()), 3);
  RC_CHECK_EQ(count_valid({}), 0);
}

RC_TEST("the mean keeps its fractional part") {
  // The check that catches an integer starting value in accumulate. These two
  // average to 21.0, and an int accumulator reports 20.
  const std::vector<Reading> two = {{20.5, 0, true}, {21.5, 0, true}};
  RC_CHECK_NEAR(mean_celsius(two), 21.0, 1e-12);
}

RC_TEST("the mean of a batch is correct") {
  const std::vector<Reading> readings = {{1.5, 0, true}, {2.5, 0, true}, {3.5, 0, true}};
  RC_CHECK_NEAR(mean_celsius(readings), 2.5, 1e-12);
}

RC_TEST("the mean of nothing is zero rather than a division by zero") {
  RC_CHECK_NEAR(mean_celsius({}), 0.0, 1e-12);
}

RC_TEST("conversion produces one value per reading") {
  const std::vector<double> converted = to_fahrenheit(batch());
  RC_CHECK_EQ(converted.size(), std::size_t{5});
}

RC_TEST("conversion is correct at the fixed points") {
  const std::vector<Reading> known = {{0.0, 0, true}, {100.0, 0, true}, {-40.0, 0, true}};
  const std::vector<double> converted = to_fahrenheit(known);
  RC_REQUIRE_EQ(converted.size(), std::size_t{3});
  RC_CHECK_NEAR(converted[0], 32.0, 1e-12);
  RC_CHECK_NEAR(converted[1], 212.0, 1e-12);
  RC_CHECK_NEAR(converted[2], -40.0, 1e-12);   // the one temperature that agrees
}

RC_TEST("converting nothing produces nothing") {
  RC_CHECK(to_fahrenheit({}).empty());
}

RC_TEST("the comparator is false for a reading against itself") {
  // Irreflexivity, which is the part of a strict weak ordering that std::sort
  // actually depends on. A comparator written with <= fails here, and would
  // otherwise fail much later as a heap overflow inside the standard library.
  const Reading one{20.0, 100, true};
  RC_CHECK(!earlier(one, one));
}

RC_TEST("the comparator orders by time in both directions") {
  const Reading early{20.0, 100, true};
  const Reading late{20.0, 500, true};
  RC_CHECK(earlier(early, late));
  RC_CHECK(!earlier(late, early));
}

RC_TEST("sorting puts readings in time order") {
  std::vector<Reading> readings = batch();
  sort_by_time(readings);

  RC_REQUIRE_EQ(readings.size(), std::size_t{5});
  for (std::size_t i = 1; i < readings.size(); ++i) {
    RC_CHECK(readings[i - 1].at_ms <= readings[i].at_ms);
  }
  RC_CHECK_EQ(readings.front().at_ms, 100L);
  RC_CHECK_EQ(readings.back().at_ms, 500L);
}

RC_TEST("sorting many equal timestamps is safe") {
  // A comparator using <= walks off the end of the range hunting a pivot that
  // cannot exist. Duplicates are what bring that out, and under the sanitizers
  // this test is where it surfaces.
  std::vector<Reading> readings;
  for (int i = 0; i < 500; ++i) {
    readings.push_back({static_cast<double>(i), static_cast<long>(i % 3), true});
  }
  sort_by_time(readings);

  RC_CHECK_EQ(readings.size(), std::size_t{500});
  for (std::size_t i = 1; i < readings.size(); ++i) {
    RC_CHECK(readings[i - 1].at_ms <= readings[i].at_ms);
  }
}

RC_TEST("dropping invalid readings actually shortens the vector") {
  // The check that catches remove_if called without erase. Without the erase
  // the size never changes and nothing reports an error.
  std::vector<Reading> readings = batch();
  drop_invalid(readings);
  RC_CHECK_EQ(readings.size(), std::size_t{3});
}

RC_TEST("dropping leaves only valid readings, in their original order") {
  std::vector<Reading> readings = batch();
  drop_invalid(readings);

  RC_REQUIRE_EQ(readings.size(), std::size_t{3});
  for (const Reading& reading : readings) RC_CHECK(reading.valid);
  RC_CHECK_EQ(readings[0].at_ms, 300L);
  RC_CHECK_EQ(readings[1].at_ms, 100L);
  RC_CHECK_EQ(readings[2].at_ms, 400L);
}

RC_TEST("dropping from a batch with nothing to drop changes nothing") {
  std::vector<Reading> readings = {{1.0, 1, true}, {2.0, 2, true}};
  drop_invalid(readings);
  RC_CHECK_EQ(readings.size(), std::size_t{2});
}

RC_TEST("dropping everything leaves an empty vector") {
  std::vector<Reading> readings = {{1.0, 1, false}, {2.0, 2, false}};
  drop_invalid(readings);
  RC_CHECK(readings.empty());
}
