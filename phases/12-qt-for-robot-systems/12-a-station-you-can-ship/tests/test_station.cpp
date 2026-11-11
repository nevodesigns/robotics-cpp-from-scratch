// The framework's main is suppressed because the event loop needs a QCoreApplication to exist before it does.
#define RC_TEST_NO_MAIN
#include <rc/test/rc_test.hpp>

#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QEventLoop>
#include <QtCore/QFile>
#include <QtCore/QTemporaryDir>
#include <QtCore/QTimer>
#include <QtNetwork/QHostAddress>
#include <QtNetwork/QTcpServer>
#include <QtNetwork/QTcpSocket>

#include <functional>
#include <iostream>
#include <memory>
#include <vector>

#include <rc/core/qt_licence.hpp>
#include <rc/qt/deploy_check.hpp>

#include "solution.hpp"

namespace {

bool wait_for(const std::function<bool()>& condition, int ms = 2000) {
  QEventLoop loop;
  QTimer poll;
  QTimer limit;
  limit.setSingleShot(true);
  QObject::connect(&poll, &QTimer::timeout, &loop, [&] {
    if (condition()) loop.quit();
  });
  QObject::connect(&limit, &QTimer::timeout, &loop, &QEventLoop::quit);
  poll.start(5);
  limit.start(ms);
  loop.exec();
  return condition();
}

void pump(int ms) {
  QEventLoop loop;
  QTimer::singleShot(ms, &loop, &QEventLoop::quit);
  loop.exec();
}

// A robot for the station to talk to, on this machine: it says HB every 20 ms while it is beating, and answers each CMD with ACK, unless told to lose the first answer to each.
struct Robot {
  QTcpServer server;
  std::unique_ptr<QTcpSocket> socket;
  QTimer beat;
  QByteArray buffer;
  std::vector<std::pair<int, double>> received;       // every CMD that arrived, with its number and speed
  rc::core::CommandReceiver receiver{1024};
  int applied = 0;                                    // commands applied by the receiver: each number once
  bool beating = true;
  bool lose_first_ack = false;
  int lines_after_mark = 0;
  bool marked = false;

  Robot() {
    server.listen(QHostAddress::LocalHost, 0);
    QObject::connect(&server, &QTcpServer::newConnection, [this] {
      socket.reset(server.nextPendingConnection());
      QObject::connect(socket.get(), &QTcpSocket::readyRead, [this] { read(); });
    });
    QObject::connect(&beat, &QTimer::timeout, [this] {
      if (beating && socket) socket->write("HB\n");
    });
    beat.start(20);
  }
  quint16 port() const { return server.serverPort(); }
  bool connected() const { return socket != nullptr; }

  void read() {
    buffer.append(socket->readAll());
    int end = buffer.indexOf('\n');
    while (end >= 0) {
      const QString line = QString::fromUtf8(buffer.left(end));
      buffer.remove(0, end + 1);
      if (marked) ++lines_after_mark;
      const QStringList parts = line.split(' ');
      if (parts.size() == 3 && parts[0] == "CMD") {
        const int seq = parts[1].toInt();
        received.push_back({seq, parts[2].toDouble()});
        int arrivals = 0;
        for (const auto& r : received)
          if (r.first == seq) ++arrivals;
        if (receiver.accept(static_cast<std::uint16_t>(seq))) ++applied;
        if (!(lose_first_ack && arrivals == 1)) socket->write(QByteArray("ACK ") + QByteArray::number(seq) + "\n");
      }
      end = buffer.indexOf('\n');
    }
  }

  double last_speed() const { return received.empty() ? -1.0 : received.back().second; }
};

}  // namespace

RC_TEST("the station connects, hears the robot, and drives at zero with the key up and at the limit with it held") {
  Robot robot;
  Station station(QHostAddress::LocalHost, robot.port(), 1.5);
  station.start();
  RC_CHECK(wait_for([&] { return robot.connected() && station.heard(); }));
  RC_CHECK(station.health() == rc::qt::Health::kAlive);

  const double up = station.drive(1.0);
  RC_CHECK(wait_for([&] { return robot.received.size() >= 1; }));
  RC_CHECK_NEAR(up, 0.0, 1e-12);
  RC_CHECK_NEAR(robot.last_speed(), 0.0, 1e-12);       // a zero is sent: it is a command to stop

  station.set_enable(true);
  const double held = station.drive(1.0);
  const double too_fast = station.drive(9.0);
  RC_CHECK(wait_for([&] { return robot.received.size() >= 3; }));
  std::cout << "  key up: " << up << " m/s sent; key held: " << held << " for a request of 1 and " << too_fast << " for a request of 9 (limit 1.5); the robot got " << robot.received.size()
            << " commands, the last at " << robot.last_speed() << "\n\n";
  RC_CHECK_NEAR(held, 1.0, 1e-12);
  RC_CHECK_NEAR(too_fast, 1.5, 1e-12);
  RC_CHECK_NEAR(robot.last_speed(), 1.5, 1e-9);
  RC_CHECK(wait_for([&] { return station.pending() == 0; }));          // every command was answered
  station.stop();
}

RC_TEST("a robot that goes quiet is lost and the speed is zero, and when it comes back the station does not move it until the key is pressed again") {
  Robot robot;
  Station station(QHostAddress::LocalHost, robot.port(), 1.5);
  station.start();
  RC_CHECK(wait_for([&] { return station.heard(); }));
  station.set_enable(true);
  RC_CHECK_NEAR(station.drive(1.0), 1.0, 1e-12);

  robot.beating = false;                                 // the robot hangs
  const bool lost = wait_for([&] { return station.health() == rc::qt::Health::kLost; });
  const double while_lost = station.drive(1.0);
  RC_CHECK(lost);
  RC_CHECK_NEAR(while_lost, 0.0, 1e-12);

  robot.beating = true;                                  // and recovers, with the key still held
  const bool alive = wait_for([&] { return station.health() == rc::qt::Health::kAlive; });
  const double after_return = station.drive(1.0);
  std::cout << "  robot silent: lost " << lost << ", speed " << while_lost << "; robot back with the key still held: alive " << alive << ", speed " << after_return << "\n";
  RC_CHECK(alive);
  RC_CHECK_NEAR(after_return, 0.0, 1e-12);             // it does not start by itself

  station.set_enable(false);
  station.set_enable(true);                              // let go and press again
  const double after_press = station.drive(1.0);
  std::cout << "  after the key was let go and pressed: " << after_press << "\n\n";
  RC_CHECK_NEAR(after_press, 1.0, 1e-12);
  station.stop();
}

RC_TEST("a command whose answer is lost is sent again and applied once") {
  Robot robot;
  robot.lose_first_ack = true;
  Station station(QHostAddress::LocalHost, robot.port(), 1.5);
  station.start();
  RC_CHECK(wait_for([&] { return station.heard(); }));
  station.set_enable(true);
  station.drive(1.0);
  RC_CHECK(wait_for([&] { return robot.received.size() >= 2 && station.pending() == 0; }));
  std::cout << "  the first answer lost: the robot received " << robot.received.size() << " copies of the command, applied " << robot.applied << ", the station has " << station.pending()
            << " waiting and " << station.failed() << " failed\n\n";
  RC_CHECK(robot.received.size() >= 2);
  RC_CHECK_EQ(robot.applied, 1);                       // however often it arrived
  RC_CHECK_EQ(station.pending(), 0u);
  RC_CHECK_EQ(station.failed(), 0u);
  station.stop();
}

RC_TEST("stop closes the retries, the monitor and the link in the reverse of the order they were started, once, and nothing is sent after") {
  Robot robot;
  Station station(QHostAddress::LocalHost, robot.port(), 1.5);
  station.start();
  RC_CHECK(wait_for([&] { return station.heard(); }));
  station.set_enable(true);
  station.drive(1.0);
  RC_CHECK(wait_for([&] { return station.pending() == 0; }));

  const rc::qt::ShutdownReport report = station.stop();
  std::cout << "  stop closed:";
  for (const QString& n : report.order) std::cout << " " << n.toStdString();
  std::cout << "\n";
  RC_CHECK(report.ok());
  RC_CHECK((report.order == QStringList() << "retries" << "monitor" << "link"));
  const rc::qt::ShutdownReport again = station.stop();
  RC_CHECK_EQ(again.order.size(), 0);                  // asked twice, done once

  robot.marked = true;
  const int before = static_cast<int>(robot.received.size());
  station.drive(1.0);                                    // nothing goes out on a link that is closed
  pump(150);
  std::cout << "  after stop: " << robot.received.size() - before << " commands reached the robot\n\n";
  RC_CHECK_EQ(static_cast<int>(robot.received.size()), before);
}

RC_TEST("the station is shippable: the modules it links owe nothing more, and a folder that holds its parts passes the check") {
  const rc::core::Assessment a = rc::core::assess(std::vector<std::string>{Station::modules()[0].toStdString(), Station::modules()[1].toStdString()}, rc::core::Linking::kDynamic, true, true);
  std::cout << "  the modules " << Station::modules().join(", ").toStdString() << ", dynamic, with the licence text and an offer of source: " << a.findings.size() << " findings\n";
  RC_CHECK(a.ok());
  RC_CHECK_EQ(Station::modules().size(), 2);

  // A station that has not been started has no link and no key held: it allows no speed, and a station that is shippable is one that is safe when nothing has been done to it.
  Station idle(QHostAddress::LocalHost, 1, 1.5);
  RC_CHECK_NEAR(idle.drive(1.0), 0.0, 1e-12);
  RC_CHECK(!idle.armed());

  QTemporaryDir dir;
  auto touch = [&](const QString& rel) {
    QDir().mkpath(QFileInfo(dir.path() + "/" + rel).absolutePath());
    QFile f(dir.path() + "/" + rel);
    if (f.open(QIODevice::WriteOnly)) f.write("x");
  };
  touch("station");
  touch("libQt6Core.so.6");
  touch("libQt6Network.so.6");
  touch("LICENSE.txt");
  const rc::qt::DeployReport ok = rc::qt::check_deployment(dir.path(), QStringList() << "station*" << "*Qt6Core*" << "*Qt6Network*" << "LICENSE*", QStringList());
  RC_CHECK(ok.ok());
  QFile::remove(dir.path() + "/libQt6Network.so.6");
  const rc::qt::DeployReport missing = rc::qt::check_deployment(dir.path(), QStringList() << "station*" << "*Qt6Core*" << "*Qt6Network*" << "LICENSE*", QStringList());
  RC_CHECK_EQ(missing.missing.size(), 1);
  if (missing.missing.size() == 1) RC_CHECK(missing.missing[0] == "*Qt6Network*");
}

int main(int argc, char** argv) {
  qputenv("QT_QPA_PLATFORM", "offscreen");
  QCoreApplication app(argc, argv);
  return rc::test::run_all();
}

#include "test_station.moc"
