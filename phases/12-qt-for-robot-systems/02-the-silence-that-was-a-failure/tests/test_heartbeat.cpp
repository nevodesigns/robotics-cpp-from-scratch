// The framework's main is suppressed because the event loop needs a QCoreApplication to exist before it does.
#define RC_TEST_NO_MAIN
#include <rc/test/rc_test.hpp>

#include <QtCore/QCoreApplication>
#include <QtCore/QElapsedTimer>
#include <QtCore/QEventLoop>
#include <QtCore/QTimer>
#include <QtNetwork/QHostAddress>
#include <QtNetwork/QTcpServer>
#include <QtNetwork/QTcpSocket>

#include <functional>
#include <iostream>
#include <memory>

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

}  // namespace

RC_TEST("a socket that is connected to a server that says nothing is still connected, and nothing in it says anything is wrong") {
  QTcpServer server;
  RC_CHECK(server.listen(QHostAddress::LocalHost, 0));
  QTcpSocket socket;
  socket.connectToHost(QHostAddress::LocalHost, server.serverPort());
  RC_CHECK(wait_for([&] { return socket.state() == QAbstractSocket::ConnectedState; }));
  RC_CHECK(wait_for([&] { return server.hasPendingConnections(); }));
  std::unique_ptr<QTcpSocket> peer(server.nextPendingConnection());
  pump(300);
  std::cout << "  300 ms of silence from a server that is up: the socket is " << (socket.state() == QAbstractSocket::ConnectedState ? "connected" : "not connected") << ", " << socket.bytesAvailable()
            << " bytes arrived\n";
  RC_CHECK(socket.state() == QAbstractSocket::ConnectedState);
  RC_CHECK_EQ(socket.bytesAvailable(), 0);

  // The watchdog of this lesson is what says it. A link that has not been heard for longer than the limit is lost, though it is open.
  Watchdog dog(100, 300, 0);
  RC_CHECK(dog.health(50) == Health::kAlive);
  RC_CHECK(dog.health(300) == Health::kLost);
}

RC_TEST("a watchdog is alive, then late, then lost, by the age of the last sign, and a sign makes it alive again") {
  Watchdog dog(100, 300, 1000);                  // started at 1000 ms
  RC_CHECK(dog.health(1000) == Health::kAlive);
  RC_CHECK(dog.health(1099) == Health::kAlive);
  RC_CHECK(dog.health(1100) == Health::kLate);   // exactly the limit is late
  RC_CHECK(dog.health(1299) == Health::kLate);
  RC_CHECK(dog.health(1300) == Health::kLost);
  RC_CHECK(dog.health(900000) == Health::kLost);
  RC_CHECK_EQ(dog.age(1250), 250);

  dog.feed(1400);
  RC_CHECK(dog.health(1400) == Health::kAlive);
  RC_CHECK(dog.health(1499) == Health::kAlive);
  RC_CHECK(dog.health(1500) == Health::kLate);
  RC_CHECK_EQ(dog.age(1500), 100);

  // A link that never said anything starts with the grace of its limits, and does not begin lost.
  Watchdog silent(100, 300, 5000);
  RC_CHECK(silent.health(5050) == Health::kAlive);
  RC_CHECK(silent.health(5300) == Health::kLost);

  // With no room between the limits there is no late state.
  Watchdog blunt(200, 200, 0);
  RC_CHECK(blunt.health(199) == Health::kAlive);
  RC_CHECK(blunt.health(200) == Health::kLost);
  Watchdog backwards(300, 100, 0);
  RC_CHECK(backwards.health(99) == Health::kAlive);
  RC_CHECK(backwards.health(100) == Health::kLost);
}

RC_TEST("a clock that is set back is no age at all, and does not make a link lost or alive by mistake") {
  Watchdog dog(100, 300, 0);
  dog.feed(10000);
  const long long behind = dog.age(9000);          // the wall clock was put back a second
  std::cout << "  the last sign at 10000 and a time of 9000: age " << behind << "\n";
  RC_CHECK_EQ(behind, 0);
  RC_CHECK(dog.health(9000) == Health::kAlive);
  RC_CHECK_EQ(dog.age(10000), 0);
  RC_CHECK_EQ(dog.age(-5), 0);
  // And forward by an hour, which a link that said nothing for an hour deserves.
  RC_CHECK(dog.health(10000 + 3600000) == Health::kLost);
  RC_CHECK_EQ(dog.age(10000 + 3600000), 3600000);
  RC_CHECK_EQ(dog.age(4000000000000LL), 4000000000000LL - 10000);          // no overflow in a long run
}

RC_TEST("the monitor says it was late and lost no earlier than the limits, once each, and alive again as soon as it is told") {
  HeartbeatMonitor monitor(60, 150, 10);
  QElapsedTimer clock;
  int emitted = 0;
  qint64 late_at = -1, lost_at = -1;
  QObject::connect(&monitor, &HeartbeatMonitor::health_changed, [&](int h) {
    ++emitted;
    if (h == static_cast<int>(Health::kLate)) late_at = clock.elapsed();
    if (h == static_cast<int>(Health::kLost)) lost_at = clock.elapsed();
  });
  clock.start();
  monitor.start();
  RC_CHECK(wait_for([&] { return monitor.health() == Health::kLost; }));
  pump(60);                                          // many more looks at a link that stays lost
  std::cout << "  silence: late after " << late_at << " ms, lost after " << lost_at << " ms, " << emitted << " signals for many looks\n";
  RC_CHECK(late_at == -1 || late_at >= 60);          // never before its limit (a stalled machine can look once, late, and go straight to lost)
  RC_CHECK(lost_at >= 150);
  RC_CHECK(emitted >= 1 && emitted <= 2);            // late and then lost, and nothing more however many times it looked
  RC_CHECK_EQ(monitor.changes().size(), emitted);    // one signal for each change
  RC_CHECK(!monitor.changes().isEmpty());
  if (!monitor.changes().isEmpty()) RC_CHECK_EQ(monitor.changes().last(), static_cast<int>(Health::kLost));

  monitor.beat();
  std::cout << "  a beat: " << (monitor.health() == Health::kAlive ? "alive at once" : "not alive") << "\n\n";
  RC_CHECK(monitor.health() == Health::kAlive);      // news at once, without waiting for a look
  const QList<int> changes = monitor.changes();
  RC_CHECK_EQ(changes.size(), emitted);
  RC_CHECK(changes.size() >= 2);
  if (changes.size() >= 2) {
    RC_CHECK_EQ(changes[changes.size() - 2], static_cast<int>(Health::kLost));
    RC_CHECK_EQ(changes.last(), static_cast<int>(Health::kAlive));
  }
  monitor.stop();
}

RC_TEST("a robot that keeps sending is never late, and one that stops is, with the changes in order") {
  HeartbeatMonitor monitor(80, 200, 10);
  QTimer sender;
  QObject::connect(&sender, &QTimer::timeout, [&] { monitor.beat(); });
  monitor.start();
  sender.start(25);
  pump(350);                                         // beats every 25 ms for 350 ms
  RC_CHECK(monitor.health() == Health::kAlive);
  RC_CHECK_EQ(monitor.changes().size(), 0);          // not one change while it kept talking

  sender.stop();                                     // the robot hangs
  RC_CHECK(wait_for([&] { return monitor.health() == Health::kLost; }));
  const QList<int> after_hang = monitor.changes();
  std::cout << "  talking for 350 ms: no changes; then silence: ";
  for (int c : after_hang) std::cout << c << " ";
  std::cout << "(1 is late, 2 is lost)\n\n";
  RC_CHECK(after_hang.size() >= 1 && after_hang.size() <= 2);
  if (!after_hang.isEmpty()) RC_CHECK_EQ(after_hang.last(), static_cast<int>(Health::kLost));
  if (after_hang.size() == 2) RC_CHECK_EQ(after_hang[0], static_cast<int>(Health::kLate));
  monitor.stop();
}

int main(int argc, char** argv) {
  qputenv("QT_QPA_PLATFORM", "offscreen");
  QCoreApplication app(argc, argv);
  return rc::test::run_all();
}
