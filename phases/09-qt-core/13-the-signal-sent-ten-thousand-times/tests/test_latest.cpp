#define RC_TEST_NO_MAIN
#include <rc/test/rc_test.hpp>

#include <QtCore/QCoreApplication>
#include <QtCore/QEventLoop>
#include <QtCore/QObject>
#include <QtCore/QThread>
#include <QtCore/QTimer>

#include <iostream>
#include <thread>

#include "solution.hpp"

namespace {

// A sensor with a plain signal, which is what a first version connects to the interface.
class Sensor : public QObject {
  Q_OBJECT
 public:
  void emit_reading(double v) { emit reading(v); }
 signals:
  void reading(double value);
};

void run_for(int ms) {
  QEventLoop loop;
  QTimer::singleShot(ms, &loop, &QEventLoop::quit);
  loop.exec();
}

}  // namespace

RC_TEST("a queued signal sent ten thousand times is ten thousand calls, all of them after the sender is done") {
  Sensor sensor;
  QObject receiver;
  long long calls = 0;
  double last = -1.0;
  QObject::connect(&sensor, &Sensor::reading, &receiver, [&](double v) {
    ++calls;
    last = v;
  }, Qt::QueuedConnection);

  for (int i = 0; i < 10000; ++i) sensor.emit_reading(static_cast<double>(i));
  const long long calls_before_loop = calls;
  QCoreApplication::processEvents();

  std::cout << "  10000 queued signals: " << calls_before_loop << " calls before the loop ran, " << calls << " after; the last value was " << last << "\n\n";
  RC_CHECK_EQ(calls_before_loop, 0);
  RC_CHECK_EQ(calls, 10000);                      // none were merged
  RC_CHECK_EQ(static_cast<int>(last), 9999);

  // And the same writes into the latest value are one call.
  LatestValue latest;
  long long updates = 0;
  QObject::connect(&latest, &LatestValue::updated, &receiver, [&](double, long long) { ++updates; });
  for (int i = 0; i < 10000; ++i) latest.publish(static_cast<double>(i));
  QCoreApplication::processEvents();
  RC_CHECK_EQ(updates, 1);
}

RC_TEST("ten thousand writes before the loop runs are one update, with the newest value and a count of what it replaced") {
  LatestValue latest;
  QObject receiver;
  int updates = 0;
  double value = -1.0;
  long long skipped = -1;
  QObject::connect(&latest, &LatestValue::updated, &receiver, [&](double v, long long s) {
    ++updates;
    value = v;
    skipped = s;
  });

  for (int i = 0; i < 10000; ++i) latest.publish(static_cast<double>(i));
  RC_CHECK_EQ(updates, 0);                         // delivery is on the event loop, not inside publish
  QCoreApplication::processEvents();

  std::cout << "  10000 writes: " << updates << " update, value " << value << ", skipped " << skipped << ", published " << latest.published() << "\n\n";
  RC_CHECK_EQ(updates, 1);
  RC_CHECK_EQ(static_cast<int>(value), 9999);
  RC_CHECK_EQ(skipped, 9999);
  RC_CHECK_EQ(latest.published(), 10000);
  RC_CHECK_EQ(latest.updates(), 1);
}

RC_TEST("a write after a delivery needs a new delivery, and each update counts only what came since the last") {
  LatestValue latest;
  QObject receiver;
  double values[8] = {};
  long long skips[8] = {};
  int n = 0;
  QObject::connect(&latest, &LatestValue::updated, &receiver, [&](double v, long long s) {
    if (n < 8) {
      values[n] = v;
      skips[n] = s;
    }
    ++n;
  });

  latest.publish(1.0);
  latest.publish(2.0);
  latest.publish(3.0);
  QCoreApplication::processEvents();
  latest.publish(4.0);
  latest.publish(5.0);
  QCoreApplication::processEvents();
  latest.publish(6.0);
  QCoreApplication::processEvents();

  std::cout << "  3 writes, 2 writes, 1 write, each followed by the loop: " << n << " updates, skipped " << skips[0] << ", " << skips[1] << ", " << skips[2]
            << "\n\n";
  RC_CHECK_EQ(n, 3);
  if (n == 3) {
    RC_CHECK_NEAR(values[0], 3.0, 1e-12);
    RC_CHECK_NEAR(values[1], 5.0, 1e-12);
    RC_CHECK_NEAR(values[2], 6.0, 1e-12);
    RC_CHECK_EQ(skips[0], 2);
    RC_CHECK_EQ(skips[1], 1);
    RC_CHECK_EQ(skips[2], 0);                      // a lone write replaced nothing
  }
}

RC_TEST("writes from another thread are all accounted for, and the last value always arrives") {
  LatestValue latest;
  QObject receiver;
  long long updates = 0;
  long long skipped_total = 0;
  double last = -1.0;
  QObject::connect(&latest, &LatestValue::updated, &receiver, [&](double v, long long s) {
    ++updates;
    skipped_total += s;
    last = v;
  });

  const int writes = 200000;
  std::thread producer([&] {
    for (int i = 1; i <= writes; ++i) latest.publish(static_cast<double>(i));
  });
  // The loop runs while the producer writes, so some updates are delivered during the run and some after.
  run_for(30);
  producer.join();
  run_for(30);

  std::cout << "  " << writes << " writes from another thread: " << updates << " updates, " << skipped_total << " skipped, " << updates + skipped_total
            << " accounted for\n\n";
  RC_CHECK_EQ(latest.published(), writes);
  RC_CHECK(updates >= 1);
  RC_CHECK(updates <= writes);
  RC_CHECK_EQ(updates + skipped_total, static_cast<long long>(writes));    // every write is an update or was counted as replaced
  RC_CHECK_EQ(static_cast<int>(last), writes);                              // and the newest is never the one lost
}

RC_TEST("a direct connection runs the slot on the sensor's thread, and the latest value runs it on the interface's") {
  QThread* const main_thread = QThread::currentThread();
  Sensor sensor;
  QThread* direct_ran_on = nullptr;
  QObject::connect(&sensor, &Sensor::reading, [&](double) { direct_ran_on = QThread::currentThread(); });   // no receiver: direct

  LatestValue latest;
  QObject receiver;
  QThread* latest_ran_on = nullptr;
  QObject::connect(&latest, &LatestValue::updated, &receiver, [&](double, long long) { latest_ran_on = QThread::currentThread(); });

  std::thread producer([&] {
    sensor.emit_reading(1.0);
    latest.publish(1.0);
  });
  producer.join();
  run_for(20);

  std::cout << "  sensor thread is " << (direct_ran_on != main_thread ? "not " : "") << "the interface's: a direct slot ran on "
            << (direct_ran_on == main_thread ? "the interface's thread" : "the sensor's thread") << ", the latest value's slot on "
            << (latest_ran_on == main_thread ? "the interface's thread" : "another thread") << "\n\n";
  RC_CHECK(direct_ran_on != nullptr);
  RC_CHECK(direct_ran_on != main_thread);          // the trap: a plain connect from another thread runs where it was emitted
  RC_CHECK(latest_ran_on == main_thread);
}

int main(int argc, char** argv) {
  qputenv("QT_QPA_PLATFORM", "offscreen");
  QCoreApplication app(argc, argv);
  return rc::test::run_all();
}

#include "test_latest.moc"
