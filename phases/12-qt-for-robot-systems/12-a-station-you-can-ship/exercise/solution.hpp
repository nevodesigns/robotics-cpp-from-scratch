#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <QtCore/QElapsedTimer>
#include <QtCore/QHash>
#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QTimer>
#include <QtNetwork/QHostAddress>
#include <rc/core/command_channel.hpp>
#include <rc/qt/heartbeat.hpp>
#include <rc/qt/interlock.hpp>
#include <rc/qt/link_client.hpp>
#include <rc/qt/shutdown.hpp>

// Phase 12 ends with a control station that can be shipped. Each lesson of the phase made one part of it safe, and the station is where the parts meet:
//
//   a link that comes back after it drops                    12-01   the station keeps trying, and waits longer after each failure
//   a watchdog that notices a robot that has gone quiet      12-02   the robot says HB, and the station is told when it stops
//   commands that are retried and applied once               12-03   every command has a number, and is sent again until it is answered
//   an enable key and an interlock                           12-08   speed only while the key is held, the link is alive, and never by itself
//   a shutdown in the reverse order of starting              12-11   the link, the watchdog and the retries are closed, once
//
// The robot of this protocol sends a line HB now and then, and a line ACK <number> for each command. The station sends CMD <number> <speed>. The speed that is sent is the speed that
// the interlock allows, which is zero unless everything is as it should be, and a command is sent even then: a zero is a command to stop, and it has to arrive.

class Station : public QObject {
  Q_OBJECT

 public:
  // The modules of Qt that this station links, for the check of what it owes (lesson 12-10).
  static QStringList modules() { return QStringList() << QStringLiteral("Core") << QStringLiteral("Network"); }

  Station(const QHostAddress& host, quint16 port, double limit_mps, QObject* parent = nullptr)
      : QObject(parent), link_(host, port, 20, 200, 100), monitor_(100, 300, 10), interlock_(limit_mps), sender_(50, 5) {
    connect(&link_, &rc::qt::LinkClient::line_received, this, &Station::on_line);
    connect(&monitor_, &rc::qt::HeartbeatMonitor::health_changed, this, &Station::on_health);
    connect(&retry_, &QTimer::timeout, this, &Station::retry);
    retry_.setInterval(10);
  }

  // Starts everything, and records how to close each as it is started.
  void start() {
    clock_.start();
    link_.start();
    shutdown_.add(QStringLiteral("link"), [this] {
      link_.stop();
      return true;
    });
    monitor_.start();
    shutdown_.add(QStringLiteral("monitor"), [this] {
      monitor_.stop();
      return true;
    });
    retry_.start();
    shutdown_.add(QStringLiteral("retries"), [this] {
      retry_.stop();
      return true;
    });
  }

  // The enable key is held or not (from the keyboard, lesson 12-08).
  void set_enable(bool held) { interlock_.set_enable(held); }

  // Asks to drive at `requested_mps`. What is sent to the robot is what the interlock allows, which is returned. The command is numbered and kept until the robot answers it.
  double drive(double requested_mps) {
    // TODO: speed = interlock_.permitted(requested_mps). seq = sender_.send(clock_.elapsed()). speeds_.insert(seq, speed). link_.send_line(command_line(seq, speed)). ++sent_.
    // Return the speed.
    (void)requested_mps;
    return -1.0;
  }

  // Closes everything in the reverse order of starting, once. The report says what ran and what failed.
  rc::qt::ShutdownReport stop() {
    const rc::qt::ShutdownReport report = shutdown_.run();
    rc::qt::flush_deferred_deletes();
    return report;
  }

  bool heard() const { return heard_; }                                       // the robot has said something since the station started
  rc::qt::Health health() const { return monitor_.health(); }
  std::size_t pending() const { return sender_.pending(); }                  // commands not answered yet
  std::size_t failed() const { return sender_.failed().size(); }             // commands that were never answered
  int sent() const { return sent_; }                                         // commands that were drive()n, not counting the retries
  bool armed() const { return interlock_.armed(); }

 signals:
  void health_changed(int health);

 private slots:
  // A line from the robot: HB is a sign of life, and ACK <number> answers a command.
  void on_line(const QString& line) {
    // TODO: the line "HB": heard_ = true; monitor_.beat(); interlock_.set_link_alive(true). A line that starts with "ACK ": the number after it, with toUInt(&ok); if ok and not above 65535,
    // sender_.ack(that number as a std::uint16_t) and speeds_.remove(it). Any other line is ignored.
    (void)line;
  }

  // The robot is alive, late or lost. Only alive, and only once it has been heard, lets the interlock allow speed.
  void on_health(int health) {
    // TODO: interlock_.set_link_alive(heard_ && health == static_cast<int>(rc::qt::Health::kAlive)); then emit health_changed(health).
    (void)health;
  }

  // Every 10 ms: the commands that were not answered in time are sent again, with the speed they had.
  void retry() {
    // TODO: for each seq of sender_.due(clock_.elapsed()): link_.send_line(command_line(seq, speeds_.value(seq, 0.0))).
  }

 private:
  static QString command_line(std::uint16_t seq, double speed) { return QStringLiteral("CMD %1 %2").arg(seq).arg(speed, 0, 'f', 3); }

  rc::qt::LinkClient link_;
  rc::qt::HeartbeatMonitor monitor_;
  rc::qt::Interlock interlock_;
  rc::core::CommandSender sender_;
  rc::qt::ShutdownSequence shutdown_;
  QHash<std::uint16_t, double> speeds_;
  QTimer retry_;
  QElapsedTimer clock_;
  bool heard_ = false;
  int sent_ = 0;
};

#endif  // LESSON_SOLUTION_HPP
