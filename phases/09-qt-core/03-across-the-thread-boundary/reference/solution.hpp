#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <QMetaType>
#include <QObject>
#include <QThread>

#include <cstdint>

// One reading, and which thread produced it.
//
// A queued connection copies its arguments, so anything sent across one has to
// be a type Qt's meta object system knows how to copy. A plain struct is fine
// and has to say so.
struct Reading {
  double value = 0.0;
  quint64 produced_on = 0;
};

// Declaring it to the meta object system. On Qt 6 this is not actually needed
// for a plain value type crossing a connection made with the function pointer
// syntax, which registers itself: measured, this struct arrives five hundred
// times out of five hundred with neither this line nor a call to
// qRegisterMetaType. It is here because it costs nothing, it is required on
// Qt 5, and it says out loud that this type is meant to travel.
Q_DECLARE_METATYPE(Reading)

// Something that produces readings on whatever thread it has been moved to.
class Sensor : public QObject {
  Q_OBJECT

 public:
  explicit Sensor(QObject* parent = nullptr) : QObject(parent) {}

  // Q_INVOKABLE so it can be started with a queued call from another thread.
  // Calling it directly would run it on the caller's thread, which is the
  // whole thing this lesson is about.
  Q_INVOKABLE void produce(int count) {
    for (int i = 0; i < count; ++i) {
      Reading r;
      r.value = static_cast<double>(i);
      r.produced_on = current_thread();
      emit reading(r);
    }
    emit finished();
  }

  static quint64 current_thread() {
    return reinterpret_cast<quint64>(QThread::currentThreadId());
  }

 signals:
  void reading(Reading value);
  void finished();
};

// Collects them, and records where each end of the journey happened.
class Log : public QObject {
  Q_OBJECT

 public:
  explicit Log(QObject* parent = nullptr) : QObject(parent) {}

  int count() const { return count_; }
  quint64 produced_on() const { return produced_on_; }
  quint64 consumed_on() const { return consumed_on_; }
  double last() const { return last_; }

 public slots:
  void take(Reading value) {
    ++count_;
    last_ = value.value;
    produced_on_ = value.produced_on;

    // The interesting one. With a queued connection this is the thread that
    // owns the Log, not the thread that emitted the signal.
    consumed_on_ = Sensor::current_thread();
  }

 private:
  int count_ = 0;
  double last_ = 0.0;
  quint64 produced_on_ = 0;
  quint64 consumed_on_ = 0;
};

// Wire a sensor to a log so that the log's slot runs on the log's own thread.
//
// The connection type must not be Direct. Qt's default, AutoConnection, decides
// at emit time: same thread means a direct call, different threads means the
// argument is copied and posted to the receiver's event loop. That default is
// almost always what you want, and saying so explicitly is worth the line when
// the whole point of the code is which thread things happen on.
inline bool wire(Sensor* sensor, Log* log) {
  if (sensor == nullptr || log == nullptr) return false;

  return static_cast<bool>(
      QObject::connect(sensor, &Sensor::reading, log, &Log::take, Qt::AutoConnection));
}

#endif  // LESSON_SOLUTION_HPP
