#define RC_TEST_NO_MAIN
#include <rc/test/rc_test.hpp>

#include <QtCore/QCoreApplication>
#include <QtCore/QElapsedTimer>
#include <QtCore/QEventLoop>
#include <QtCore/QThread>
#include <QtCore/QTimer>

#include <iostream>
#include <memory>

#include "solution.hpp"

namespace {

void run_for(int ms) {
  QEventLoop loop;
  QTimer::singleShot(ms, &loop, &QEventLoop::quit);
  loop.exec();
}

}  // namespace

RC_TEST("a sensor on its own thread reaches the interface on the interface's thread, and every reading is accounted for") {
  QThread* const interface_thread = QThread::currentThread();
  SensorBridge bridge(1000);
  long long updates = 0;
  long long skipped_total = 0;
  double last = 0.0;
  QThread* shown_on = nullptr;
  QObject::connect(&bridge, &SensorBridge::shown, [&](double v, long long s) {
    ++updates;
    skipped_total += s;
    last = v;
    shown_on = QThread::currentThread();
  });

  bridge.start();
  run_for(100);
  bridge.stop();
  run_for(20);                       // lets the last update that was waiting be delivered

  std::cout << "  a 1 kHz sensor for 100 ms: " << bridge.produced() << " readings, " << updates << " updates, " << skipped_total << " skipped; produced on "
            << (bridge.producer_thread() != interface_thread ? "another thread" : "the interface's thread") << ", shown on "
            << (shown_on == interface_thread ? "the interface's thread" : "another thread") << "\n\n";
  RC_CHECK(bridge.produced() >= 1);
  RC_CHECK(bridge.producer_thread() != nullptr);
  RC_CHECK(bridge.producer_thread() != interface_thread);       // it did run somewhere else
  RC_CHECK(updates >= 1);
  RC_CHECK(shown_on == interface_thread);                       // and the interface only heard of it on its own thread
  RC_CHECK_EQ(bridge.published(), bridge.produced());
  RC_CHECK_EQ(updates + skipped_total, bridge.produced());      // every reading is an update or counted as replaced
  RC_CHECK_EQ(static_cast<long long>(last), bridge.produced()); // and the newest is the one shown last
}

RC_TEST("after stop nothing more is produced, and a bridge that is destroyed while running stops first") {
  long long after_stop = -1;
  long long later = -1;
  {
    SensorBridge bridge(1000);
    bridge.start();
    run_for(50);
    bridge.stop();
    RC_CHECK(bridge.produced() >= 1);
    after_stop = bridge.produced();
    run_for(50);
    later = bridge.produced();
  }
  std::cout << "  produced " << after_stop << " at stop and " << later << " fifty milliseconds later\n";
  RC_CHECK_EQ(later, after_stop);

  // Destroyed while running: the destructor must ask and wait, or the thread outlives what it uses.
  int destroyed = 0;
  for (int i = 0; i < 20; ++i) {
    std::unique_ptr<SensorBridge> bridge(new SensorBridge(1000));
    bridge->start();
    QThread::msleep(3);
    bridge.reset();
    ++destroyed;
  }
  std::cout << "  " << destroyed << " bridges destroyed while their thread was running\n\n";
  RC_CHECK_EQ(destroyed, 20);

  SensorBridge never_started(100);
  never_started.stop();              // safe: nothing to wait for
  never_started.stop();              // and safe twice
  RC_CHECK_EQ(never_started.produced(), 0);
}

RC_TEST("the producer runs no faster than its rate, and stop is answered within a period or two") {
  SensorBridge bridge(100);          // a period of 10 ms
  RC_CHECK_EQ(bridge.period_ms(), 10);
  QElapsedTimer wall;
  wall.start();
  bridge.start();
  run_for(200);
  QElapsedTimer stopping;
  stopping.start();
  bridge.stop();
  const long long stop_ms = stopping.elapsed();
  const long long run_ms = wall.elapsed();

  const long long ceiling = run_ms / 10 + 2;          // a reading, then a sleep of at least 10 ms, so it cannot go faster
  std::cout << "  100 Hz for " << run_ms << " ms: " << bridge.produced() << " readings (at most " << ceiling << "); stop took " << stop_ms << " ms\n\n";
  RC_CHECK(bridge.produced() >= 1);
  RC_CHECK(bridge.produced() <= ceiling);
  RC_CHECK(stop_ms < 500);                             // not a wait for the run to end
}

int main(int argc, char** argv) {
  qputenv("QT_QPA_PLATFORM", "offscreen");
  QCoreApplication app(argc, argv);
  return rc::test::run_all();
}
