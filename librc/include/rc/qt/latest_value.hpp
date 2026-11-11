// rc/qt/latest_value.hpp
//
// A latest value that posts one update to its own thread and counts what it replaced, from lesson 09-13, graduated.
//
// 10000 queued emits were 10000 calls after the loop ran. 10000 writes to a LatestValue before the loop were 1 update with the value 9999 and 9999 skipped. From a second thread, updates plus the sum of skipped equalled
// the writes, and the last value written was the last delivered. A direct connection from another thread ran its slot on the sensor's thread, and the update ran on the interface's.

#ifndef RC_QT_LATEST_VALUE
#define RC_QT_LATEST_VALUE

#include <QMetaObject>
#include <QObject>

#include <mutex>

namespace rc {
namespace qt {

// A sensor that produces ten thousand readings a second, connected to the interface with a queued signal, makes ten thousand calls
// into the interface. Qt does not merge them. Each is an event in a queue, each runs the slot, and if the slot is slower than the sensor the
// queue grows for as long as the sensor runs.
//
// The interface wants the latest value, not every value, and it wants to know how many it did not see. LatestValue is written from any
// thread, keeps only the newest value, posts ONE event to the thread it lives on while one is not already waiting, and says in each update
// how many writes that update replaced.

class LatestValue : public QObject {
  Q_OBJECT

 public:
  explicit LatestValue(QObject* parent = nullptr) : QObject(parent) {}

  // Callable from any thread. Stores the value, and makes sure one delivery is waiting on this object's own thread.
  void publish(double value) {
    bool post = false;
    {
      std::lock_guard<std::mutex> lock(mutex_);
      value_ = value;
      ++published_;
      post = !pending_;
      pending_ = true;
    }
    if (post) QMetaObject::invokeMethod(this, &LatestValue::deliver, Qt::QueuedConnection);
  }

  // Writes made, and updates delivered. Callable from any thread.
  long long published() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return published_;
  }
  long long updates() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return updates_;
  }

 signals:
  // `skipped` is how many writes since the last update were replaced by this one's value. Every write is either an update's value or
  // counted as skipped: over a run, the updates plus the sum of skipped equal the writes made.
  void updated(double value, long long skipped);

 private:
  void deliver() {
    double value = 0.0;
    long long skipped = 0;
    {
      std::lock_guard<std::mutex> lock(mutex_);
      pending_ = false;                 // a write from now on needs a new delivery
      value = value_;
      skipped = published_ - seen_ - 1;
      seen_ = published_;
      ++updates_;
    }
    emit updated(value, skipped);
  }

  mutable std::mutex mutex_;
  double value_ = 0.0;
  long long published_ = 0;
  long long seen_ = 0;
  long long updates_ = 0;
  bool pending_ = false;
};

}  // namespace qt
}  // namespace rc

#endif  // RC_QT_LATEST_VALUE
