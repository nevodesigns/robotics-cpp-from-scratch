// A QObject needs an event loop to receive a queued signal, and an event loop
// needs an application, so the framework's own main is suppressed.
#define RC_TEST_NO_MAIN
#include <rc/test/rc_test.hpp>

#include <QCoreApplication>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QMetaObject>
#include <QThread>
#include <QTimer>

#include <iostream>

#include "solution.hpp"

namespace {

// Runs the receiver's event loop until the log has what it is waiting for, or
// until the deadline.
//
// Waiting on a count rather than on a completion signal, deliberately. A signal
// connected after the work has already finished waits for something that has
// been and gone, which is a race in the test rather than in the code, and it
// showed up here as a suite that passed six times out of seven.
//
// The deadline is what turns a mistake into a failing test rather than a suite
// that hangs, which lesson 08-05 met from the other side.
bool pump_until(const Log& log, int expected, int milliseconds) {
  QElapsedTimer clock;
  clock.start();
  while (log.count() < expected && clock.elapsed() < milliseconds) {
    QCoreApplication::processEvents(QEventLoop::AllEvents, 5);
  }
  return log.count() >= expected;
}

// A sensor living on its own thread, cleaned up whatever the test does.
class Worker {
 public:
  Worker() {
    sensor.moveToThread(&thread);
    thread.start();
  }

  ~Worker() {
    thread.quit();
    thread.wait();
  }

  Worker(const Worker&) = delete;
  Worker& operator=(const Worker&) = delete;

  void start(int count) {
    QMetaObject::invokeMethod(&sensor, "produce", Qt::QueuedConnection, Q_ARG(int, count));
  }

  Sensor sensor;
  QThread thread;
};

quint64 this_thread() { return Sensor::current_thread(); }

// A pair carrying a type Qt does not register for itself, so that the failure
// this lesson warns about can be watched rather than described. Qt::HANDLE is a
// typedef for void*, and the meta object system looks it up by that name.
class RawSender : public QObject {
  Q_OBJECT
 public:
  Q_INVOKABLE void send(int count) {
    for (int i = 0; i < count; ++i) emit raw(QThread::currentThreadId());
  }
 signals:
  void raw(Qt::HANDLE handle);
};

class RawSink : public QObject {
  Q_OBJECT
 public:
  int received = 0;
 public slots:
  void take(Qt::HANDLE) { ++received; }
};

}  // namespace

// The two classes above declare Q_OBJECT inside a .cpp rather than a header, so
// the code moc generates for them lands in a file named after this one and has
// to be included. A header gets this for free because the build lists it as a
// source; a .cpp does not. Lesson 09-02 is about what is in that file.
#include "test_threads.moc"

RC_TEST("wiring reports whether it managed to connect") {
  Sensor sensor;
  Log log;
  RC_CHECK(wire(&sensor, &log));
  RC_CHECK(!wire(nullptr, &log));
  RC_CHECK(!wire(&sensor, nullptr));
}

RC_TEST("on one thread the slot runs immediately, in the emitter") {
  // With both objects on the same thread there is no queue and no copy: the
  // connection is a function call, and it has happened by the time emit
  // returns. That is Qt's default deciding at emit time.
  Sensor sensor;
  Log log;
  RC_REQUIRE(wire(&sensor, &log));

  sensor.produce(3);

  RC_CHECK_EQ(log.count(), 3);
  RC_CHECK_EQ(log.consumed_on(), this_thread());
  RC_CHECK_EQ(log.produced_on(), this_thread());
}

RC_TEST("across threads every reading arrives") {
  // The check that catches an unregistered argument type. A queued connection
  // copies what it carries, and a type the meta object system does not know is
  // dropped: the connection succeeds, the signal is emitted, Qt prints one line
  // on the console, and not a single delivery happens.
  Worker worker;
  Log log;
  RC_REQUIRE(wire(&worker.sensor, &log));

  worker.start(500);
  RC_REQUIRE(pump_until(log, 500, 5000));

  RC_CHECK_EQ(log.count(), 500);
  RC_CHECK_NEAR(log.last(), 499.0, 1e-12);
}

RC_TEST("across threads the slot runs on the receiver's thread, not the sender's") {
  // The reason any of this exists. The reading is produced on the worker and
  // consumed on the thread that owns the Log, which is how a widget can be
  // updated from data that arrived somewhere else without ever being touched
  // from that somewhere else.
  Worker worker;
  Log log;
  RC_REQUIRE(wire(&worker.sensor, &log));

  worker.start(50);
  RC_REQUIRE(pump_until(log, 50, 5000));

  RC_REQUIRE(log.count() > 0);
  RC_CHECK(log.produced_on() != this_thread());
  RC_CHECK_EQ(log.consumed_on(), this_thread());

  std::cout << "\n  produced on the worker thread, consumed on the thread that owns the log\n";
}

RC_TEST("an object moved to a thread has not moved the things that connect to it") {
  // Affinity belongs to the object, not to the connection. The sensor is on the
  // worker and the log is not, and neither had to be told about the other.
  Worker worker;
  Log log;

  RC_CHECK(worker.sensor.thread() == &worker.thread);
  RC_CHECK(log.thread() == QThread::currentThread());
}

RC_TEST("nothing is delivered while the receiver's event loop is not running") {
  // Worth seeing, because it is the shape of a great many bugs. A queued
  // delivery is an event posted to the receiver's loop, so a receiver whose
  // loop is not running has not lost the readings and has not received them
  // either: they are waiting.
  Worker worker;
  Log log;
  RC_REQUIRE(wire(&worker.sensor, &log));

  worker.start(20);
  QThread::msleep(50);           // the worker has finished producing by now

  const int before_pumping = log.count();
  RC_CHECK_EQ(before_pumping, 0);

  RC_REQUIRE(pump_until(log, 20, 5000));
  RC_CHECK_EQ(log.count(), 20);
}

RC_TEST("two sensors on two threads both reach one log") {
  Worker first;
  Worker second;
  Log log;
  RC_REQUIRE(wire(&first.sensor, &log));
  RC_REQUIRE(wire(&second.sensor, &log));

  first.start(100);
  second.start(100);
  RC_REQUIRE(pump_until(log, 200, 5000));

  RC_CHECK_EQ(log.count(), 200);
  RC_CHECK_EQ(log.consumed_on(), this_thread());
}

RC_TEST("an argument type Qt cannot queue is dropped in silence") {
  // Not a defect in Qt and not a rare corner: it is what the failure looks
  // like. The connection succeeds. The signal is emitted. Every delivery is
  // discarded, and the only evidence is one line on the console that nothing
  // in the program can see.
  //
  // Which is the durable lesson here, now that Qt 6 registers ordinary types by
  // itself: a connection that was made is not a connection that delivers, and
  // the only way to know is to check that something arrived.
  RawSender sender;
  RawSink sink;
  QThread thread;
  sender.moveToThread(&thread);
  thread.start();

  RC_REQUIRE(static_cast<bool>(
      QObject::connect(&sender, &RawSender::raw, &sink, &RawSink::take)));

  QMetaObject::invokeMethod(&sender, "send", Qt::QueuedConnection, Q_ARG(int, 100));

  QElapsedTimer clock;
  clock.start();
  while (clock.elapsed() < 300) QCoreApplication::processEvents(QEventLoop::AllEvents, 5);

  thread.quit();
  thread.wait();

  std::cout << "  a signal carrying Qt::HANDLE delivered " << sink.received
            << " of 100 across a thread boundary\n";
  RC_CHECK_EQ(sink.received, 0);
}

int main(int argc, char** argv) {
  QCoreApplication app(argc, argv);
  return rc::test::run_all();
}
