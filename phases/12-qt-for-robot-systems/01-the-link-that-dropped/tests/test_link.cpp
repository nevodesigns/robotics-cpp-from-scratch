// The framework's main is suppressed because the event loop needs a QCoreApplication to exist before it does.
#define RC_TEST_NO_MAIN
#include <rc/test/rc_test.hpp>

#include <QtCore/QCoreApplication>
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

// Runs the event loop until the condition holds or `ms` has passed. Returns whether it held.
bool wait_for(const std::function<bool()>& condition, int ms = 1500) {
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

// A port that nothing is listening on.
quint16 free_port() {
  QTcpServer probe;
  probe.listen(QHostAddress::LocalHost, 0);
  const quint16 port = probe.serverPort();
  probe.close();
  return port;
}

}  // namespace

RC_TEST("the wait doubles for each failure up to a cap, and a large number of failures does not overflow") {
  RC_CHECK_EQ(next_delay_ms(0, 100, 5000), 100);
  RC_CHECK_EQ(next_delay_ms(1, 100, 5000), 200);
  RC_CHECK_EQ(next_delay_ms(2, 100, 5000), 400);
  RC_CHECK_EQ(next_delay_ms(5, 100, 5000), 3200);
  RC_CHECK_EQ(next_delay_ms(6, 100, 5000), 5000);          // 6400 would be more
  RC_CHECK_EQ(next_delay_ms(7, 100, 5000), 5000);
  RC_CHECK_EQ(next_delay_ms(100, 100, 5000), 5000);        // no overflow
  RC_CHECK_EQ(next_delay_ms(1000000, 1, 2000000000), 2000000000);
  RC_CHECK_EQ(next_delay_ms(-3, 100, 5000), 100);          // an attempt before the first is the first
  RC_CHECK_EQ(next_delay_ms(4, 0, 5000), 0);               // a base that is not positive is no wait
  RC_CHECK_EQ(next_delay_ms(4, -50, 5000), 0);
  RC_CHECK_EQ(next_delay_ms(3, 100, 50), 100);             // a cap below the base is the base
}

RC_TEST("a connection that is refused is retried after waits that grow, and a success starts them again from the base") {
  const quint16 port = free_port();
  LinkClient client(QHostAddress::LocalHost, port, 20, 1000, 100);
  client.start();
  const bool three = wait_for([&] { return client.delays().size() >= 3; });
  const QList<int> delays = client.delays();
  std::cout << "  no server: the waits scheduled were";
  for (int d : delays) std::cout << " " << d;
  std::cout << " ms\n";
  RC_CHECK(three);
  if (delays.size() >= 3) {
    RC_CHECK_EQ(delays[0], 20);
    RC_CHECK_EQ(delays[1], 40);
    RC_CHECK_EQ(delays[2], 80);
  }
  RC_CHECK(client.attempts() >= 3);

  QTcpServer server;
  RC_CHECK(server.listen(QHostAddress::LocalHost, port));
  const bool connected = wait_for([&] { return client.state() == LinkClient::kConnected; });
  std::cout << "  the server appeared: connected " << connected << ", failures counted since then " << client.attempts() << ", good connections " << client.connects() << "\n\n";
  RC_CHECK(connected);
  RC_CHECK_EQ(client.attempts(), 0);
  RC_CHECK_EQ(client.connects(), 1);
  client.stop();
}

RC_TEST("a link that the server closes is one drop and one retry, though it is reported twice, and the client reconnects") {
  QTcpServer server;
  RC_CHECK(server.listen(QHostAddress::LocalHost, 0));
  LinkClient client(QHostAddress::LocalHost, server.serverPort(), 20, 1000, 100);
  client.start();
  RC_CHECK(wait_for([&] { return client.state() == LinkClient::kConnected; }));
  RC_CHECK(wait_for([&] { return server.hasPendingConnections(); }));
  std::unique_ptr<QTcpSocket> peer(server.nextPendingConnection());
  RC_CHECK(peer != nullptr);
  if (!peer) return;
  RC_CHECK_EQ(client.delays().size(), 0);

  peer->disconnectFromHost();
  const bool noticed = wait_for([&] { return client.state() != LinkClient::kConnected; });
  pump(10);                                           // time for the second report of the same drop, if there is one
  std::cout << "  the server closed the link: noticed " << noticed << ", waits scheduled " << client.delays().size() << ", the first " << (client.delays().isEmpty() ? -1 : client.delays().first())
            << " ms\n";
  RC_CHECK(noticed);
  RC_CHECK_EQ(client.delays().size(), 1);             // not two, though the socket said it twice
  if (!client.delays().isEmpty()) RC_CHECK_EQ(client.delays().first(), 20);

  const bool again = wait_for([&] { return client.state() == LinkClient::kConnected; });
  std::cout << "  and then: reconnected " << again << ", good connections " << client.connects() << "\n\n";
  RC_CHECK(again);
  RC_CHECK_EQ(client.connects(), 2);
  RC_CHECK_EQ(client.attempts(), 0);
  client.stop();
}

RC_TEST("lines that arrive in pieces are one line each, and the client says it is connected") {
  QTcpServer server;
  RC_CHECK(server.listen(QHostAddress::LocalHost, 0));
  LinkClient client(QHostAddress::LocalHost, server.serverPort(), 20, 1000, 100);
  client.start();
  RC_CHECK(wait_for([&] { return client.state() == LinkClient::kConnected; }));
  RC_CHECK(wait_for([&] { return server.hasPendingConnections(); }));
  std::unique_ptr<QTcpSocket> peer(server.nextPendingConnection());
  RC_CHECK(peer != nullptr);
  if (!peer) return;
  peer->write("temp 2");
  peer->flush();
  pump(30);
  RC_CHECK_EQ(client.lines().size(), 0);              // half a line is not a line
  peer->write("1.5\nvolt 12");
  peer->flush();
  pump(30);
  peer->write(".1\n");
  peer->flush();
  RC_CHECK(wait_for([&] { return client.lines().size() >= 2; }));
  const QStringList lines = client.lines();
  RC_CHECK_EQ(lines.size(), 2);
  RC_CHECK(client.send_line("ping"));                 // and the other way: a line to the robot
  RC_CHECK(wait_for([&] { return peer->bytesAvailable() >= 5; }));
  RC_CHECK(peer->readAll() == "ping\n");
  if (lines.size() == 2) {
    RC_CHECK_EQ(lines[0].toStdString(), std::string("temp 21.5"));
    RC_CHECK_EQ(lines[1].toStdString(), std::string("volt 12.1"));
  }
  client.stop();
}

RC_TEST("stop ends the retries for good, even when the server comes back") {
  const quint16 port = free_port();
  LinkClient client(QHostAddress::LocalHost, port, 20, 1000, 100);
  client.start();
  RC_CHECK(wait_for([&] { return client.delays().size() >= 1; }));
  client.stop();
  const int scheduled = client.delays().size();
  RC_CHECK(client.state() == LinkClient::kStopped);
  RC_CHECK(!client.send_line("x"));                   // nothing is sent when there is no link

  QTcpServer server;
  RC_CHECK(server.listen(QHostAddress::LocalHost, port));
  pump(200);                                          // long enough for several retries, if any were still going
  std::cout << "  stopped after " << scheduled << " waits; the server came back and 200 ms went by: " << client.delays().size() << " waits, state " << client.state() << ", good connections "
            << client.connects() << "\n\n";
  RC_CHECK_EQ(client.delays().size(), scheduled);
  RC_CHECK(client.state() == LinkClient::kStopped);
  RC_CHECK_EQ(client.connects(), 0);
  RC_CHECK(!server.hasPendingConnections());

  client.start();                                     // and it can be started again
  RC_CHECK(wait_for([&] { return client.state() == LinkClient::kConnected; }));
  client.stop();
}

int main(int argc, char** argv) {
  qputenv("QT_QPA_PLATFORM", "offscreen");
  QCoreApplication app(argc, argv);
  return rc::test::run_all();
}

