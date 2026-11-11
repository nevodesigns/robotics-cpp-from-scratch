#include <rc/test/rc_test.hpp>

#include <iostream>
#include <string>
#include <vector>

#include "solution.hpp"

RC_TEST("a message reaches every subscriber of its topic, in the order they subscribed, and nobody else") {
  TopicBus bus;
  std::vector<std::string> log;
  bus.subscribe("scan", [&](const std::string&, const std::string& p) { log.push_back("a:" + p); });
  bus.subscribe("scan", [&](const std::string&, const std::string& p) { log.push_back("b:" + p); });
  bus.subscribe("pose", [&](const std::string&, const std::string& p) { log.push_back("pose:" + p); });
  const int delivered = bus.publish("scan", "1");
  std::cout << "  published scan: " << delivered << " delivered, log";
  for (const std::string& s : log) std::cout << " " << s;
  std::cout << "\n\n";
  RC_CHECK(delivered == 2);
  RC_CHECK(log.size() == 2);
  RC_CHECK(log.size() == 2 && log[0] == "a:1" && log[1] == "b:1");
  RC_CHECK(bus.subscriber_count("scan") == 2);
  RC_CHECK(bus.subscriber_count("pose") == 1);
  RC_CHECK(bus.subscriber_count("nothing") == 0);
}

RC_TEST("a topic with nobody listening drops the message, counts it, and is not an error") {
  TopicBus bus;
  RC_CHECK(bus.publish("scan", "1") == 0);
  RC_CHECK(bus.publish("scan", "2") == 0);
  RC_CHECK(bus.dropped() == 2);
  int heard = 0;
  bus.subscribe("scan", [&](const std::string&, const std::string&) { ++heard; });
  RC_CHECK(bus.publish("scan", "3") == 1);
  RC_CHECK(heard == 1);
  RC_CHECK(bus.dropped() == 2);                       // the first two stay lost: a late subscriber does not see them
  std::cout << "  two messages with nobody listening: dropped " << bus.dropped() << "; a subscriber who arrived after heard " << heard << " of 3\n\n";
}

RC_TEST("unsubscribing stops delivery, and says whether there was anything to stop") {
  TopicBus bus;
  int heard = 0;
  const TopicBus::Token t = bus.subscribe("x", [&](const std::string&, const std::string&) { ++heard; });
  RC_CHECK(bus.publish("x", "1") == 1);
  RC_CHECK(bus.unsubscribe(t));
  RC_CHECK(!bus.unsubscribe(t));
  RC_CHECK(!bus.unsubscribe(9999));
  RC_CHECK(bus.publish("x", "2") == 0);
  RC_CHECK(heard == 1);
  RC_CHECK(bus.subscriber_count("x") == 0);
}

RC_TEST("a handler may unsubscribe itself or a later subscriber in the middle of a delivery, and nothing is skipped or called after it left") {
  TopicBus bus;
  std::vector<std::string> log;
  TopicBus::Token a = 0;
  TopicBus::Token c = 0;
  TopicBus::Token b = 0;
  a = bus.subscribe("x", [&](const std::string&, const std::string&) {
    log.push_back("a");
    bus.unsubscribe(a);                                // leaves after its first message
    bus.unsubscribe(b);                                // and takes b with it before b is reached
  });
  b = bus.subscribe("x", [&](const std::string&, const std::string&) { log.push_back("b"); });
  c = bus.subscribe("x", [&](const std::string&, const std::string&) { log.push_back("c"); });
  const int first = bus.publish("x", "1");
  const int second = bus.publish("x", "2");
  std::cout << "  first message: " << first << " called, second: " << second << " called, log";
  for (const std::string& s : log) std::cout << " " << s;
  std::cout << "\n\n";
  RC_CHECK(first == 2);                                // a and c: b was removed before its turn
  RC_CHECK(second == 1);                               // c alone
  RC_CHECK(log.size() == 3);
  RC_CHECK(log.size() == 3 && log[0] == "a" && log[1] == "c" && log[2] == "c");
  RC_CHECK(c != 0);
}

RC_TEST("a message published from inside a handler waits its turn: every subscriber of the first message is called before the second starts") {
  TopicBus bus;
  std::vector<std::string> log;
  bus.subscribe("in", [&](const std::string&, const std::string& p) {
    log.push_back("first heard " + p);
    const int immediate = bus.publish("out", p);
    log.push_back("publish inside returned " + std::to_string(immediate));
  });
  bus.subscribe("in", [&](const std::string&, const std::string& p) { log.push_back("second heard " + p); });
  bus.subscribe("out", [&](const std::string&, const std::string& p) { log.push_back("out heard " + p); });
  const int n = bus.publish("in", "7");
  std::cout << "  order of events:\n";
  for (const std::string& s : log) std::cout << "    " << s << "\n";
  std::cout << "\n";
  RC_CHECK(n == 2);
  RC_CHECK(log.size() == 4);
  RC_CHECK(log.size() == 4 && log[0] == "first heard 7");
  RC_CHECK(log.size() == 4 && log[1] == "publish inside returned 0");
  RC_CHECK(log.size() == 4 && log[2] == "second heard 7");
  RC_CHECK(log.size() == 4 && log[3] == "out heard 7");
}

RC_TEST("two topics that publish to each other make a loop that the queue turns into a long run and not a stack overflow") {
  TopicBus bus;
  int ping = 0;
  int pong = 0;
  bus.subscribe("ping", [&](const std::string&, const std::string&) {
    ++ping;
    if (ping < 5000) bus.publish("pong", "x");
  });
  bus.subscribe("pong", [&](const std::string&, const std::string&) {
    ++pong;
    if (pong < 5000) bus.publish("ping", "x");
  });
  bus.publish("ping", "x");
  std::cout << "  a ping and a pong that answer each other: " << ping << " pings and " << pong << " pongs, delivered without nesting\n\n";
  RC_CHECK(ping == 5000);
  RC_CHECK(pong == 4999);
}
