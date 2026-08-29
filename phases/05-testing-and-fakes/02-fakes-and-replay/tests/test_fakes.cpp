#include <rc/test/rc_test.hpp>

#include <memory>
#include <vector>

#include "solution.hpp"

namespace {

// A short trace standing in for a recorded session. In a real lesson this comes
// from a file captured off a device; the shape of the test does not change.
std::vector<double> warm_trace() { return {20.0, 20.5, 21.0, 21.5, 22.0}; }

}  // namespace

RC_TEST("a replay sensor answers its trace in order") {
  ReplaySensor sensor(warm_trace());
  for (const double expected : warm_trace()) {
    const auto reading = sensor.read(0);
    RC_REQUIRE(reading.has_value());
    RC_CHECK_NEAR(reading.value(), expected, 1e-12);
  }
}

RC_TEST("a replay sensor reports the end of its trace") {
  ReplaySensor sensor({1.0});
  RC_CHECK(sensor.read(0).has_value());

  const auto past_end = sensor.read(0);
  RC_REQUIRE(!past_end.has_value());
  RC_CHECK(past_end.error() == ReadError::EndOfData);
}

RC_TEST("a replay sensor counts how many times it was asked") {
  ReplaySensor sensor(warm_trace());
  sensor.read(0);
  sensor.read(0);
  RC_CHECK_EQ(sensor.reads(), std::size_t{2});
}

RC_TEST("a failed read still counts as a read") {
  ReplaySensor sensor({1.0});
  sensor.fail_at(0, ReadError::Timeout);
  sensor.read(0);
  RC_CHECK_EQ(sensor.reads(), std::size_t{1});
}

RC_TEST("a chosen reading can be made to fail") {
  // The whole reason a fake beats real hardware: the interesting case is
  // available on demand rather than on the device's schedule.
  ReplaySensor sensor(warm_trace());
  sensor.fail_at(2, ReadError::Corrupt);

  RC_CHECK(sensor.read(0).has_value());
  RC_CHECK(sensor.read(0).has_value());

  const auto third = sensor.read(0);
  RC_REQUIRE(!third.has_value());
  RC_CHECK(third.error() == ReadError::Corrupt);

  // The trace continues afterwards, so a single dropout can be tested without
  // ending the session.
  RC_CHECK(sensor.read(0).has_value());
}

RC_TEST("the monitor averages the readings it received") {
  ReplaySensor sensor({10.0, 20.0, 30.0});
  SensorMonitor monitor(2);
  for (int i = 0; i < 3; ++i) monitor.poll(sensor, i);

  RC_CHECK_EQ(monitor.good(), 3);
  RC_CHECK_NEAR(monitor.mean(), 20.0, 1e-12);
  RC_CHECK(!monitor.stale());
}

RC_TEST("the mean of nothing is zero rather than a division by zero") {
  const SensorMonitor monitor(2);
  RC_CHECK_NEAR(monitor.mean(), 0.0, 1e-12);
}

RC_TEST("a failed reading does not enter the average") {
  ReplaySensor sensor({10.0, 30.0});
  sensor.fail_at(1, ReadError::Corrupt);

  SensorMonitor monitor(2);
  monitor.poll(sensor, 0);
  monitor.poll(sensor, 1);

  RC_CHECK_EQ(monitor.good(), 1);
  RC_CHECK_EQ(monitor.failures(), 1);
  RC_CHECK_NEAR(monitor.mean(), 10.0, 1e-12);
}

RC_TEST("the monitor remembers what went wrong") {
  ReplaySensor sensor({1.0});
  sensor.fail_at(0, ReadError::Disconnected);

  SensorMonitor monitor(2);
  monitor.poll(sensor, 0);
  RC_CHECK(monitor.last_error() == ReadError::Disconnected);
}

RC_TEST("one dropped reading does not make a sensor stale") {
  // A sensor that misses a reading now and again is healthy. Treating every
  // dropout as a fault produces a robot that stops constantly.
  ReplaySensor sensor({10.0, 20.0, 30.0, 40.0});
  sensor.fail_at(1, ReadError::Timeout);

  SensorMonitor monitor(2);
  for (int i = 0; i < 4; ++i) monitor.poll(sensor, i);

  RC_CHECK_EQ(monitor.failures(), 1);
  RC_CHECK(!monitor.stale());
}

RC_TEST("a run of failures makes the sensor stale") {
  ReplaySensor sensor({10.0, 20.0});
  sensor.fail_at(1, ReadError::Timeout);
  sensor.fail_at(2, ReadError::Timeout);
  sensor.fail_at(3, ReadError::Timeout);

  SensorMonitor monitor(2);
  for (int i = 0; i < 4; ++i) monitor.poll(sensor, i);

  RC_CHECK_EQ(monitor.consecutive_failures(), 3);
  RC_CHECK(monitor.stale());
}

RC_TEST("a good reading clears the run") {
  ReplaySensor sensor({10.0, 20.0, 30.0, 40.0, 50.0});
  sensor.fail_at(1, ReadError::Timeout);
  sensor.fail_at(2, ReadError::Timeout);

  SensorMonitor monitor(2);
  for (int i = 0; i < 5; ++i) monitor.poll(sensor, i);

  RC_CHECK_EQ(monitor.consecutive_failures(), 0);
  RC_CHECK(!monitor.stale());
  RC_CHECK_EQ(monitor.failures(), 2);
}

RC_TEST("a sensor that stops answering entirely goes stale") {
  // The case that matters on a real robot: the device stopped, and something
  // above has to notice rather than carrying on with the last value.
  ReplaySensor sensor({10.0});
  SensorMonitor monitor(2);
  for (int i = 0; i < 6; ++i) monitor.poll(sensor, i);

  RC_CHECK(monitor.stale());
  RC_CHECK(monitor.last_error() == ReadError::EndOfData);
}

RC_TEST("the monitor works through the interface, not the fake") {
  // The seam is in the right place only if the code under test never names the
  // concrete type. Holding it through a base pointer proves that here, and is
  // what lets phase 08 swap in a real serial port with these tests unchanged.
  std::unique_ptr<SensorSource> source = std::make_unique<ReplaySensor>(warm_trace());
  SensorMonitor monitor(1);
  monitor.poll(*source, 0);
  monitor.poll(*source, 1);
  RC_CHECK_EQ(monitor.good(), 2);
}

RC_TEST("deleting through the interface destroys the whole object") {
  // Without a virtual destructor on SensorSource the derived part is never
  // destroyed, which leaks quietly and is one of the most common mistakes with
  // an interface.
  struct Counted : SensorSource {
    int* alive;
    explicit Counted(int* counter) : alive(counter) { ++*alive; }
    ~Counted() override { --*alive; }
    rc::expected<double, ReadError> read(long) override { return 1.0; }
  };

  int alive = 0;
  {
    const std::unique_ptr<SensorSource> source = std::make_unique<Counted>(&alive);
    RC_CHECK_EQ(alive, 1);
  }
  RC_CHECK_EQ(alive, 0);
}
