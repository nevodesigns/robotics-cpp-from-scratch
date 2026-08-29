#include <rc/test/rc_test.hpp>
#include <rc/test/leak_check.hpp>

#include <string>
#include <vector>

#include "solution.hpp"

namespace {
using rc::test::LeakCheck;
}

RC_TEST("a new log is empty and knows its capacity") {
  const SensorLog log(8);
  RC_CHECK_EQ(log.size(), std::size_t{0});
  RC_CHECK_EQ(log.capacity(), std::size_t{8});
  RC_CHECK(log.history().empty());
  RC_CHECK(log.names().empty());
}

RC_TEST("a recorded reading can be read back") {
  SensorLog log(8);
  log.record("imu.temp", 41.5, 1000);

  const auto found = log.latest("imu.temp");
  RC_REQUIRE(found.has_value());
  RC_CHECK_NEAR(found->value, 41.5, 1e-12);
  RC_CHECK_EQ(found->at_ms, 1000L);
}

RC_TEST("an unknown sensor answers nothing") {
  SensorLog log(8);
  log.record("imu.temp", 41.5, 1000);
  RC_CHECK(!log.latest("lidar.range").has_value());
}

RC_TEST("latest means latest") {
  SensorLog log(8);
  log.record("imu.temp", 41.5, 1000);
  log.record("imu.temp", 42.0, 1100);
  log.record("imu.temp", 40.0, 1200);

  const auto found = log.latest("imu.temp");
  RC_REQUIRE(found.has_value());
  RC_CHECK_NEAR(found->value, 40.0, 1e-12);
}

RC_TEST("names come back sorted, whatever order they arrived in") {
  SensorLog log(8);
  log.record("lidar.range", 1.0, 1);
  log.record("battery.volts", 2.0, 2);
  log.record("imu.temp", 3.0, 3);

  const std::vector<std::string> names = log.names();
  RC_REQUIRE_EQ(names.size(), std::size_t{3});
  RC_CHECK_EQ(names[0], std::string("battery.volts"));
  RC_CHECK_EQ(names[1], std::string("imu.temp"));
  RC_CHECK_EQ(names[2], std::string("lidar.range"));
}

RC_TEST("a sensor recorded twice is named once") {
  SensorLog log(8);
  log.record("imu.temp", 1.0, 1);
  log.record("imu.temp", 2.0, 2);
  RC_CHECK_EQ(log.names().size(), std::size_t{1});
}

RC_TEST("history holds what was recorded, oldest first") {
  SensorLog log(8);
  for (int i = 0; i < 3; ++i) log.record("s", static_cast<double>(i), i);

  const std::vector<Reading> history = log.history();
  RC_REQUIRE_EQ(history.size(), std::size_t{3});
  RC_CHECK_NEAR(history[0].value, 0.0, 1e-12);
  RC_CHECK_NEAR(history[2].value, 2.0, 1e-12);
}

RC_TEST("history never exceeds the capacity") {
  SensorLog log(4);
  for (int i = 0; i < 50; ++i) log.record("s", static_cast<double>(i), i);
  RC_CHECK_EQ(log.size(), std::size_t{4});
  RC_CHECK_EQ(log.history().size(), std::size_t{4});
}

RC_TEST("the oldest reading is the one dropped") {
  SensorLog log(3);
  for (int i = 0; i < 5; ++i) log.record("s", static_cast<double>(i), i);

  // Five recorded into three slots leaves the last three, in order.
  const std::vector<Reading> history = log.history();
  RC_REQUIRE_EQ(history.size(), std::size_t{3});
  RC_CHECK_NEAR(history[0].value, 2.0, 1e-12);
  RC_CHECK_NEAR(history[1].value, 3.0, 1e-12);
  RC_CHECK_NEAR(history[2].value, 4.0, 1e-12);
}

RC_TEST("order survives many wraps") {
  // This is where ring buffer arithmetic goes wrong: after wrapping several
  // times the oldest entry is not at index zero and not at next_ minus one.
  SensorLog log(5);
  for (int i = 0; i < 5000; ++i) log.record("s", static_cast<double>(i), i);

  const std::vector<Reading> history = log.history();
  RC_REQUIRE_EQ(history.size(), std::size_t{5});
  for (std::size_t i = 0; i < history.size(); ++i) {
    RC_CHECK_NEAR(history[i].value, 4995.0 + static_cast<double>(i), 1e-12);
  }
}

RC_TEST("a capacity of zero is treated as one rather than dividing by zero") {
  SensorLog log(0);
  RC_CHECK_EQ(log.capacity(), std::size_t{1});
  log.record("s", 1.0, 1);
  RC_CHECK_EQ(log.size(), std::size_t{1});
}

RC_TEST("recording does not grow the memory in use") {
  // The property the whole lesson is about. A hundred thousand readings into a
  // log with sixteen slots must leave the memory in use exactly where it was,
  // because a control loop cannot afford a pause at a moment it did not choose.
  //
  // This checks balance rather than counting zero allocations, which is the
  // honest claim the harness can make: nothing is retained, so the buffer is
  // genuinely fixed. A growing container fails it immediately.
  SensorLog log(16);
  log.record("warm.up", 0.0, 0);   // let the map allocate its first node

  const LeakCheck check;
  for (int i = 0; i < 100000; ++i) log.record("warm.up", static_cast<double>(i), i);

  RC_CHECK(check.balanced());
  RC_CHECK_EQ(log.size(), std::size_t{16});
}
