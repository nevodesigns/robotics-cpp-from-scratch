#include <rc/test/rc_test.hpp>

#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

#include "solution.hpp"

namespace {

DurableBus::Handler collect(std::vector<std::string>& into) {
  return [&into](const Stamped& m) { into.push_back(m.payload); };
}

}  // namespace

RC_TEST("a topic that is not remembered gives a late subscriber nothing of what came before it") {
  DurableBus bus;
  bus.publish("pose", "1", 0.0);
  bus.publish("pose", "2", 1.0);
  std::vector<std::string> heard;
  bus.subscribe("pose", collect(heard), 2.0, -1.0);
  RC_CHECK(heard.empty());
  bus.publish("pose", "3", 3.0);
  RC_CHECK(heard.size() == 1);
  RC_CHECK(heard.size() == 1 && heard[0] == "3");
  RC_CHECK(bus.remembered("pose") == 0);
  RC_CHECK(bus.replayed() == 0);
  std::cout << "  volatile topic, two messages before the subscriber: heard only what came after it (" << heard.size() << " message)\n\n";
}

RC_TEST("a remembered topic gives a late subscriber the last messages first, oldest first, and then what follows") {
  DurableBus bus;
  bus.advertise("map", Durability::transient_local, 3);
  for (int i = 1; i <= 5; ++i) bus.publish("map", std::to_string(i), i);
  std::vector<std::string> heard;
  bus.subscribe("map", collect(heard), 6.0, -1.0);
  bus.publish("map", "6", 6.0);
  std::cout << "  keep 3, five published, a subscriber arrives, then a sixth: heard";
  for (const std::string& s : heard) std::cout << " " << s;
  std::cout << "; replayed " << bus.replayed() << "\n\n";
  RC_CHECK(heard.size() == 4);
  RC_CHECK(heard.size() == 4 && heard[0] == "3" && heard[1] == "4" && heard[2] == "5" && heard[3] == "6");
  RC_CHECK(bus.replayed() == 3);
}

RC_TEST("what is remembered is bounded by keep however much is published, and a subscriber that was there all along hears nothing twice") {
  DurableBus bus;
  bus.advertise("status", Durability::transient_local, 1);
  std::vector<std::string> early;
  bus.subscribe("status", collect(early), 0.0, -1.0);
  for (int i = 0; i < 1000; ++i) bus.publish("status", std::to_string(i), i * 0.01);
  RC_CHECK(early.size() == 1000);                        // each once
  RC_CHECK(bus.remembered("status") == 1);
  std::vector<std::string> late;
  bus.subscribe("status", collect(late), 10.0, -1.0);
  RC_CHECK(late.size() == 1);
  RC_CHECK(late.size() == 1 && late[0] == "999");
  RC_CHECK(early.size() == 1000);                        // the late arrival did not replay to the early one
  std::cout << "  1000 published to a topic that keeps 1: remembered " << bus.remembered("status") << ", the early subscriber heard " << early.size()
            << ", the late one heard " << late.size() << " (" << (late.empty() ? "" : late[0]) << ")\n\n";
}

RC_TEST("a message remembered for a long time is old news, and a subscriber can say how old it is willing to take") {
  DurableBus bus;
  bus.advertise("goal", Durability::transient_local, 2);
  bus.publish("goal", "old", 0.0);
  bus.publish("goal", "fresh", 98.0);
  std::vector<std::string> any_age;
  std::vector<std::string> within_5;
  bus.subscribe("goal", collect(any_age), 100.0, -1.0);
  bus.subscribe("goal", collect(within_5), 100.0, 5.0);
  std::cout << "  at 100 s, remembered messages from 0 s and 98 s: no limit heard " << any_age.size() << ", a limit of 5 s heard " << within_5.size()
            << " and skipped " << bus.stale_skipped() << "\n\n";
  RC_CHECK(any_age.size() == 2);
  RC_CHECK(within_5.size() == 1);
  RC_CHECK(within_5.size() == 1 && within_5[0] == "fresh");
  RC_CHECK(bus.stale_skipped() == 1);
}

RC_TEST("a map published once is seen by every node that starts later only if the topic is remembered") {
  std::cout << "  a map is published at 0 s, three nodes start at 1, 5 and 9 s\n  topic kind          nodes that have the map\n";
  int have_volatile = -1;
  int have_remembered = -1;
  for (int kind = 0; kind < 2; ++kind) {
    DurableBus bus;
    if (kind == 1) bus.advertise("map", Durability::transient_local, 1);
    bus.publish("map", "grid", 0.0);
    std::vector<std::string> a;
    std::vector<std::string> b;
    std::vector<std::string> c;
    bus.subscribe("map", collect(a), 1.0, -1.0);
    bus.subscribe("map", collect(b), 5.0, -1.0);
    bus.subscribe("map", collect(c), 9.0, -1.0);
    const int have = (a.empty() ? 0 : 1) + (b.empty() ? 0 : 1) + (c.empty() ? 0 : 1);
    std::cout << "  " << std::left << std::setw(18) << (kind == 0 ? "not remembered" : "remembered") << std::right << std::setw(10) << have << " of 3\n";
    if (kind == 0) have_volatile = have;
    else have_remembered = have;
  }
  std::cout << "\n";
  RC_CHECK(have_volatile == 0);
  RC_CHECK(have_remembered == 3);
}
