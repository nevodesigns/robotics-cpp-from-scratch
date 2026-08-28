#include <rc/test/rc_test.hpp>

#include <QObject>
#include <vector>

#include "solution.hpp"

namespace {

// Counts what a monitor announced. Testing event driven code means recording
// the events, which needs no window and no running event loop: a direct
// connection calls straight through.
struct Recorder {
  std::vector<int> low;
  std::vector<int> recovered;

  explicit Recorder(BatteryMonitor& monitor) {
    QObject::connect(&monitor, &BatteryMonitor::lowBattery,
                     [this](int percent) { low.push_back(percent); });
    QObject::connect(&monitor, &BatteryMonitor::batteryRecovered,
                     [this](int percent) { recovered.push_back(percent); });
  }
};

}  // namespace

RC_TEST("a healthy battery announces nothing") {
  BatteryMonitor monitor(15, 20);
  Recorder events(monitor);

  monitor.update(90);
  monitor.update(60);
  monitor.update(30);

  RC_CHECK_EQ(events.low.size(), std::size_t{0});
  RC_CHECK(!monitor.isLow());
}

RC_TEST("crossing the low threshold announces once") {
  BatteryMonitor monitor(15, 20);
  Recorder events(monitor);

  monitor.update(14);

  RC_REQUIRE_EQ(events.low.size(), std::size_t{1});
  RC_CHECK_EQ(events.low[0], 14);
  RC_CHECK(monitor.isLow());
}

RC_TEST("staying low does not announce again") {
  BatteryMonitor monitor(15, 20);
  Recorder events(monitor);

  monitor.update(14);
  monitor.update(13);
  monitor.update(12);
  monitor.update(11);

  RC_CHECK_EQ(events.low.size(), std::size_t{1});
}

RC_TEST("a reading inside the band does not clear the warning") {
  BatteryMonitor monitor(15, 20);
  Recorder events(monitor);

  monitor.update(14);
  monitor.update(18);   // above low, but below clear

  RC_CHECK(monitor.isLow());
  RC_CHECK_EQ(events.recovered.size(), std::size_t{0});
}

RC_TEST("rising above the clear threshold announces recovery") {
  BatteryMonitor monitor(15, 20);
  Recorder events(monitor);

  monitor.update(14);
  monitor.update(25);

  RC_CHECK(!monitor.isLow());
  RC_REQUIRE_EQ(events.recovered.size(), std::size_t{1});
  RC_CHECK_EQ(events.recovered[0], 25);
}

RC_TEST("the monitor rearms after recovering") {
  BatteryMonitor monitor(15, 20);
  Recorder events(monitor);

  monitor.update(14);
  monitor.update(25);
  monitor.update(10);

  RC_CHECK_EQ(events.low.size(), std::size_t{2});
  RC_CHECK(monitor.isLow());
}

RC_TEST("hysteresis survives a reading that flickers on the threshold") {
  BatteryMonitor monitor(15, 20);
  Recorder events(monitor);

  monitor.update(15);
  for (int i = 0; i < 20; ++i) {
    monitor.update(15);
    monitor.update(16);
  }

  // One warning, no matter how much the reading jitters around the threshold.
  RC_CHECK_EQ(events.low.size(), std::size_t{1});
  RC_CHECK_EQ(events.recovered.size(), std::size_t{0});
}

RC_TEST("the monitor reports the last reading it was given") {
  BatteryMonitor monitor(15, 20);
  monitor.update(42);
  RC_CHECK_EQ(monitor.percent(), 42);
}
