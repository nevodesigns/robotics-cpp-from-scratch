// rc/qt/sensor_bridge.hpp
//
// A sensor on a thread of its own that reaches the interface only through a latest value, from lesson 09-14, graduated.
//
// A 1 kHz producer for 100 ms ran on another thread and every update was shown on the interface's thread; readings produced equalled readings published, and updates plus skipped equalled both. After stop() the count did not
// move, 20 bridges destroyed while running shut down cleanly, and a 100 Hz producer made no more than the run's length over 10 ms plus 2 readings.

#ifndef RC_QT_SENSOR_BRIDGE
#define RC_QT_SENSOR_BRIDGE

#include <QObject>
#include <QThread>

#include <atomic>

#include "rc/qt/latest_value.hpp"

namespace rc {
namespace qt {

// Phase 09 ends with a sensor on a thread of its own, reaching the interface without ever touching it. Everything in it is a piece of an
// earlier lesson:
//
//   a thread that only produces        09-03: it owns no interface object and calls none of their functions
//   one way across                     09-13: the only thing the thread calls is publish(), which is safe from any thread
//   the interface's thread             09-13: the update is delivered by that thread's loop, one at a time, with the count it replaced
//   a stop that joins                  07-07 and 07-14: ask, then wait, and the destructor does it too
//
// The rule that falls out: the sensor thread is a producer. It writes into a latest value and nothing else, the interface reads it
// from its own thread, and shutdown asks the producer to stop and waits for it before anything it uses is destroyed.

class SensorBridge : public QObject {
  Q_OBJECT

 public:
  explicit SensorBridge(int rate_hz, QObject* parent = nullptr)
      : QObject(parent), period_ms_(rate_hz > 0 ? 1000 / rate_hz : 1000), loop_(this) {
    if (period_ms_ < 1) period_ms_ = 1;
    connect(&latest_, &LatestValue::updated, this, &SensorBridge::shown);
  }
  ~SensorBridge() override { stop(); }

  // Starts the producer thread.
  void start() {
    loop_.start();
  }

  // Asks the producer to finish and waits for it. Safe to call twice, and when it was never started.
  void stop() {
    loop_.requestInterruption();
    loop_.wait();
  }

  int period_ms() const { return period_ms_; }

  // Readings produced, on whatever thread. Readings published equals this.
  long long produced() const { return produced_; }
  // The thread that produced, once it has produced anything.
  QThread* producer_thread() const { return producer_thread_; }
  long long updates() const { return latest_.updates(); }
  long long published() const { return latest_.published(); }

 signals:
  // On the interface's thread: the newest reading, and how many readings this one stands in for.
  void shown(double value, long long skipped);

 private:
  // One reading: the only thing the producer thread does besides waiting.
  void produce_one() {
    producer_thread_ = QThread::currentThread();
    ++produced_;
    latest_.publish(static_cast<double>(produced_));
  }

  class Loop : public QThread {
   public:
    explicit Loop(SensorBridge* bridge) : bridge_(bridge) {}

   protected:
    void run() override {
      while (!isInterruptionRequested()) {
        bridge_->produce_one();
        QThread::msleep(static_cast<unsigned long>(bridge_->period_ms_));
      }
    }

   private:
    SensorBridge* bridge_;
  };

  int period_ms_;
  LatestValue latest_;
  std::atomic<long long> produced_{0};
  std::atomic<QThread*> producer_thread_{nullptr};
  Loop loop_;
};

}  // namespace qt
}  // namespace rc

#endif  // RC_QT_SENSOR_BRIDGE
