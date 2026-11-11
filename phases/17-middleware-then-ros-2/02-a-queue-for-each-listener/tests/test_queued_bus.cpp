#include <rc/test/rc_test.hpp>

#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

#include "solution.hpp"

namespace {

std::string next_of(QueuedBus& bus, QueuedBus::Handle h) {
  std::string s;
  return bus.take(h, s) ? s : std::string("none");
}

}  // namespace

RC_TEST("a subscriber takes messages oldest first, one at a time, and finds none when it is empty") {
  QueuedBus bus;
  const QueuedBus::Handle h = bus.subscribe("scan", 3, Overflow::drop_oldest);
  bus.publish("scan", "1");
  bus.publish("scan", "2");
  bus.publish("other", "x");
  RC_CHECK(bus.pending(h) == 2);
  RC_CHECK(next_of(bus, h) == "1");
  RC_CHECK(next_of(bus, h) == "2");
  RC_CHECK(next_of(bus, h) == "none");
  RC_CHECK(bus.taken(h) == 2);
  RC_CHECK(bus.lost(h) == 0);
  std::string s;
  RC_CHECK(!bus.take(99, s));
  RC_CHECK(!bus.take(-1, s));
  RC_CHECK(bus.pending(99) == 0);
}

RC_TEST("a full queue that keeps the newest loses the oldest, and counts them") {
  QueuedBus bus;
  const QueuedBus::Handle h = bus.subscribe("pose", 3, Overflow::drop_oldest);
  for (int i = 1; i <= 5; ++i) bus.publish("pose", std::to_string(i));
  const std::string a = next_of(bus, h);
  const std::string b = next_of(bus, h);
  const std::string c = next_of(bus, h);
  std::cout << "  depth 3, five messages, keep the newest: took " << a << " " << b << " " << c << ", lost " << bus.lost(h) << "\n\n";
  RC_CHECK(a == "3");
  RC_CHECK(b == "4");
  RC_CHECK(c == "5");
  RC_CHECK(bus.lost(h) == 2);
}

RC_TEST("a full queue that keeps what it has loses the newest instead, and the choice decides which data survives") {
  QueuedBus bus;
  const QueuedBus::Handle h = bus.subscribe("log", 3, Overflow::drop_newest);
  for (int i = 1; i <= 5; ++i) bus.publish("log", std::to_string(i));
  const std::string a = next_of(bus, h);
  const std::string b = next_of(bus, h);
  const std::string c = next_of(bus, h);
  std::cout << "  depth 3, five messages, keep what is waiting: took " << a << " " << b << " " << c << ", lost " << bus.lost(h) << "\n\n";
  RC_CHECK(a == "1");
  RC_CHECK(b == "2");
  RC_CHECK(c == "3");
  RC_CHECK(bus.lost(h) == 2);
}

RC_TEST("every subscriber has its own queue, so a slow one loses messages and the fast one beside it does not") {
  QueuedBus bus;
  const QueuedBus::Handle fast = bus.subscribe("scan", 2, Overflow::drop_oldest);
  const QueuedBus::Handle slow = bus.subscribe("scan", 2, Overflow::drop_oldest);
  for (int i = 1; i <= 10; ++i) {
    bus.publish("scan", std::to_string(i));
    next_of(bus, fast);                                  // the fast one empties its queue after every message
  }
  std::cout << "  ten messages, a subscriber that keeps up and one that never takes any: fast lost " << bus.lost(fast) << " and took "
            << bus.taken(fast) << ", slow lost " << bus.lost(slow) << " and has " << bus.pending(slow) << " waiting\n\n";
  RC_CHECK(bus.taken(fast) == 10);
  RC_CHECK(bus.lost(fast) == 0);
  RC_CHECK(bus.lost(slow) == 8);
  RC_CHECK(bus.pending(slow) == 2);
}

RC_TEST("a deeper queue absorbs a burst, and does nothing for a subscriber that is slower all the time") {
  std::cout << "  a burst of 8 messages, then the subscriber takes them all\n  depth   lost\n";
  int lost_at_4 = -1;
  int lost_at_10 = -1;
  for (std::size_t depth : {1, 4, 8, 10}) {
    QueuedBus bus;
    const QueuedBus::Handle h = bus.subscribe("x", depth, Overflow::drop_oldest);
    for (int i = 0; i < 8; ++i) bus.publish("x", "m");
    while (next_of(bus, h) != "none") {}
    std::cout << "  " << std::setw(5) << depth << "   " << std::setw(4) << bus.lost(h) << "\n";
    if (depth == 4) lost_at_4 = bus.lost(h);
    if (depth == 10) lost_at_10 = bus.lost(h);
  }
  std::cout << "\n  a publisher at 100 Hz for 10 s, a subscriber that takes one every 100 ms\n  depth   lost of 1000\n";
  int lost_small = 0;
  int lost_large = 0;
  for (std::size_t depth : {1, 10, 100}) {
    QueuedBus bus;
    const QueuedBus::Handle h = bus.subscribe("x", depth, Overflow::drop_oldest);
    for (int tick = 0; tick < 1000; ++tick) {
      bus.publish("x", "m");
      if (tick % 10 == 9) next_of(bus, h);
    }
    std::cout << "  " << std::setw(5) << depth << "   " << std::setw(12) << bus.lost(h) << "\n";
    if (depth == 1) lost_small = bus.lost(h);
    if (depth == 100) lost_large = bus.lost(h);
  }
  std::cout << "\n";
  RC_CHECK(lost_at_4 == 4);
  RC_CHECK(lost_at_10 == 0);
  RC_CHECK(lost_small > 850);
  RC_CHECK(lost_large > 750);
}

RC_TEST("a depth of zero is a depth of one, and each topic has its own queues") {
  QueuedBus bus;
  const QueuedBus::Handle h = bus.subscribe("a", 0, Overflow::drop_oldest);
  const QueuedBus::Handle g = bus.subscribe("b", 5, Overflow::drop_oldest);
  bus.publish("a", "1");
  bus.publish("a", "2");
  bus.publish("b", "x");
  RC_CHECK(bus.pending(h) == 1);
  RC_CHECK(next_of(bus, h) == "2");
  RC_CHECK(bus.lost(h) == 1);
  RC_CHECK(bus.pending(g) == 1);
  RC_CHECK(next_of(bus, g) == "x");
}
