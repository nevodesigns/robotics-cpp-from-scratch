// rc/qt/heartbeat.hpp
//
// A watchdog for a link that is up and quiet, from lesson 12-02, graduated.
//
// A server that accepted a connection and said nothing left the socket connected after 300 ms and the monitor lost at its limit; the age of a heartbeat is never negative when a clock is set back; a change of health was one signal however often the monitor looked; a beat made it alive again at once.

#ifndef RC_QT_HEARTBEAT_HPP
#define RC_QT_HEARTBEAT_HPP

#include <QtCore/QElapsedTimer>
#include <QtCore/QList>
#include <QtCore/QObject>
#include <QtCore/QTimer>

namespace rc {
namespace qt {

// A link that is up is not a link that is alive. A TCP connection stays open, as far as the station can tell, for hours after the robot at the other end has hung, lost its
// power or had its cable pulled out from behind a switch: nothing closes it, and nothing arrives. A station that waits for the socket to say it has gone waits for ever.
//
// The only evidence that something is alive is that it said something recently. A heartbeat is a message that exists to say so, and a watchdog is the part that asks how
// long ago the last one came. It must be judged by a clock that does not go backwards, and it should say once when the answer changes, not at every look.

enum class Health { kAlive = 0, kLate = 1, kLost = 2 };

// A watchdog that is given the time and does not read a clock, so that it can be tested with any times at all. All times are in milliseconds on one clock.
class Watchdog {
 public:
  // The last sign of life is taken to be at `start_ms`, so that a link that never says anything has the same time to begin as one that did. It is late when the last
  // sign is `late_ms` old or more, and lost at `lost_ms`. If `lost_ms` is not more than `late_ms` there is no late state: it is alive and then lost.
  Watchdog(long long late_ms, long long lost_ms, long long start_ms) : late_(late_ms), lost_(lost_ms), last_(start_ms) {}

  // A sign of life at this time.
  void feed(long long now_ms) { last_ = now_ms; }

  long long late_ms() const { return late_; }
  long long lost_ms() const { return lost_; }

  // How long ago the last sign was. Never negative: a clock that was set back, or a time from before the last sign, is no time at all.
  long long age(long long now_ms) const { return now_ms > last_ ? now_ms - last_ : 0; }

  Health health(long long now_ms) const {
    const long long a = age(now_ms);
    if (a >= lost_) return Health::kLost;
    if (late_ < lost_ && a >= late_) return Health::kLate;
    return Health::kAlive;
  }

 private:
  long long late_;
  long long lost_;
  long long last_;
};

// The watchdog on a clock that only goes forward, looked at every `poll_ms`. A change of health is one signal, and looking again at the same health says nothing.
class HeartbeatMonitor : public QObject {
  Q_OBJECT

 public:
  HeartbeatMonitor(int late_ms, int lost_ms, int poll_ms, QObject* parent = nullptr)
      : QObject(parent), late_ms_(late_ms), lost_ms_(lost_ms), poll_ms_(poll_ms), dog_(late_ms, lost_ms, 0) {
    connect(&poll_, &QTimer::timeout, this, &HeartbeatMonitor::look);
  }

  void start() {
    clock_.start();
    dog_ = Watchdog(late_ms_, lost_ms_, 0);
    health_ = Health::kAlive;
    changes_.clear();
    poll_.start(poll_ms_);
  }
  void stop() { poll_.stop(); }

  // The robot said something.
  void beat() {
    dog_.feed(clock_.elapsed());
    look();                                         // a sign of life is news at once, and does not wait for the next look
  }

  Health health() const { return health_; }
  QList<int> changes() const { return changes_; }   // every change of health, in order, as the numbers of Health

 signals:
  void health_changed(int health);

 public slots:
  void look() {
    const Health now = dog_.health(clock_.elapsed());
    if (now == health_) return;                     // looking again at the same answer is not news
    health_ = now;
    changes_.push_back(static_cast<int>(now));
    emit health_changed(static_cast<int>(now));
  }

 private:
  int late_ms_;
  int lost_ms_;
  int poll_ms_;
  Watchdog dog_;
  Health health_ = Health::kAlive;
  QList<int> changes_;
  QElapsedTimer clock_;
  QTimer poll_;
};
}  // namespace qt
}  // namespace rc

#endif  // RC_QT_HEARTBEAT_HPP
