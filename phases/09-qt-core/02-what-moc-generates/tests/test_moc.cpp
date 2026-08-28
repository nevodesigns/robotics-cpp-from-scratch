#include <rc/test/rc_test.hpp>

#include <QMetaObject>
#include <QObject>
#include <vector>

#include "solution.hpp"

RC_TEST("moc actually ran for this class") {
  // A class whose Q_OBJECT is missing reports QObject's meta object rather than
  // its own, so its own signal is nowhere to be found.
  ThresholdSensor sensor(1.0);
  RC_CHECK_EQ(std::string(sensor.metaObject()->className()), std::string("ThresholdSensor"));
}

RC_TEST("the signal is registered in the meta object") {
  ThresholdSensor sensor(1.0);
  RC_CHECK(sensor.metaObject()->indexOfSignal("crossed(double)") >= 0);
}

RC_TEST("crossing upward announces once") {
  ThresholdSensor sensor(1.0);
  std::vector<double> seen;
  QObject::connect(&sensor, &ThresholdSensor::crossed,
                   [&seen](double value) { seen.push_back(value); });

  sensor.feed(0.5);
  sensor.feed(1.5);

  RC_REQUIRE_EQ(seen.size(), std::size_t{1});
  RC_CHECK_NEAR(seen[0], 1.5, 1e-9);
  RC_CHECK(sensor.isAbove());
}

RC_TEST("staying above does not announce again") {
  ThresholdSensor sensor(1.0);
  std::vector<double> seen;
  QObject::connect(&sensor, &ThresholdSensor::crossed,
                   [&seen](double value) { seen.push_back(value); });

  sensor.feed(1.5);
  sensor.feed(2.0);
  sensor.feed(9.0);

  RC_CHECK_EQ(seen.size(), std::size_t{1});
}

RC_TEST("dropping back rearms the sensor quietly") {
  ThresholdSensor sensor(1.0);
  std::vector<double> seen;
  QObject::connect(&sensor, &ThresholdSensor::crossed,
                   [&seen](double value) { seen.push_back(value); });

  sensor.feed(1.5);
  sensor.feed(0.2);
  RC_CHECK_EQ(seen.size(), std::size_t{1});
  RC_CHECK(!sensor.isAbove());

  sensor.feed(1.6);
  RC_CHECK_EQ(seen.size(), std::size_t{2});
}

RC_TEST("a reading exactly on the threshold is not above it") {
  ThresholdSensor sensor(1.0);
  sensor.feed(1.0);
  RC_CHECK(!sensor.isAbove());
}

RC_TEST("the sensor remembers its last reading") {
  ThresholdSensor sensor(1.0);
  sensor.feed(3.25);
  RC_CHECK_NEAR(sensor.last(), 3.25, 1e-9);
}
