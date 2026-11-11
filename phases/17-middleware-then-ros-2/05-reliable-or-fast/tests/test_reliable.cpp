#include <rc/test/rc_test.hpp>

#include <cstdint>
#include <deque>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

#include "solution.hpp"

namespace {

struct Rng {
  std::uint32_t state;
  std::uint32_t next() {
    state ^= state << 13;
    state ^= state >> 17;
    state ^= state << 5;
    return state;
  }
  double uniform() { return static_cast<double>(next() >> 8) / 16777216.0; }
};

struct InFlightData {
  int arrives;
  DataPacket packet;
};
struct InFlightAck {
  int arrives;
  int next_expected;
};

// A link: every packet is lost with probability `loss` and otherwise arrives `delay` ticks later. Acknowledgements come back the same way.
struct Outcome {
  int ticks = 0;
  int delivered = 0;
  bool in_order = true;
  int transmissions = 0;
  int retransmissions = 0;
};

Outcome run(int messages, int window, int rto, int delay, double loss, bool reliable, std::uint32_t seed) {
  Rng rng{seed};
  ReliableSender sender(window, rto);
  ReliableReceiver receiver;
  for (int i = 0; i < messages; ++i) sender.send(std::to_string(i));
  std::deque<InFlightData> data;
  std::deque<InFlightAck> acks;
  Outcome o;
  int last_unreliable_tick = 0;
  int arrived = 0;
  for (int now = 0; now < 200000; ++now) {
    // Packets arriving now.
    while (!data.empty() && data.front().arrives <= now) {
      ++arrived;
      const int ack = receiver.on_data(data.front().packet);
      data.pop_front();
      if (rng.uniform() >= loss) acks.push_back(InFlightAck{now + delay, ack});
    }
    while (!acks.empty() && acks.front().arrives <= now) {
      sender.on_ack(acks.front().next_expected);
      acks.pop_front();
    }
    for (const DataPacket& p : sender.tick(now)) {
      if (rng.uniform() >= loss) data.push_back(InFlightData{now + delay, p});
    }
    if (!reliable) {
      // Without acknowledgement the sender just forgets what it sent.
      sender.on_ack(1 << 30);
      last_unreliable_tick = now;
      if (data.empty()) break;
    }
    if (reliable && sender.idle() && data.empty() && static_cast<int>(receiver.delivered().size()) == messages) {
      o.ticks = now;
      break;
    }
  }
  if (!reliable) o.ticks = last_unreliable_tick;
  // Without acknowledgements every packet that arrives is simply used, so the count is of arrivals.
  o.delivered = reliable ? static_cast<int>(receiver.delivered().size()) : arrived;
  for (std::size_t i = 1; i < receiver.delivered().size(); ++i)
    if (std::stoi(receiver.delivered()[i]) < std::stoi(receiver.delivered()[i - 1])) o.in_order = false;
  o.transmissions = sender.transmissions();
  o.retransmissions = sender.retransmissions();
  return o;
}

}  // namespace

RC_TEST("a receiver hands messages up in order, once each, however they arrive, and acknowledges the next one it needs") {
  ReliableReceiver r;
  const int a = r.on_data(DataPacket{2, "c"});     // early
  const int b = r.on_data(DataPacket{0, "a"});     // the one it was waiting for
  const int c = r.on_data(DataPacket{1, "b"});     // fills the gap: 1 then the waiting 2
  const int d = r.on_data(DataPacket{1, "b"});     // a repeat
  const int e = r.on_data(DataPacket{0, "a"});     // another
  std::cout << "  arrivals 2, 0, 1, 1, 0: acknowledgements " << a << " " << b << " " << c << " " << d << " " << e << ", delivered";
  for (const std::string& s : r.delivered()) std::cout << " " << s;
  std::cout << ", duplicates " << r.duplicates() << "\n\n";
  RC_CHECK(a == 0);
  RC_CHECK(b == 1);
  RC_CHECK(c == 3);
  RC_CHECK(d == 3);
  RC_CHECK(e == 3);
  RC_CHECK(r.delivered().size() == 3);
  RC_CHECK(r.delivered().size() == 3 && r.delivered()[0] == "a" && r.delivered()[1] == "b" && r.delivered()[2] == "c");
  RC_CHECK(r.duplicates() == 2);
  RC_CHECK(r.expected() == 3);
  r.on_data(DataPacket{5, "f"});
  r.on_data(DataPacket{5, "f"});
  RC_CHECK(r.duplicates() == 3);                    // an early one seen twice is a duplicate as well
}

RC_TEST("a sender puts no more than its window on the wire, and sends more as acknowledgements free room") {
  ReliableSender s(4, 100);
  for (int i = 0; i < 10; ++i) s.send(std::to_string(i));
  const std::vector<DataPacket> first = s.tick(0);
  RC_CHECK(first.size() == 4);
  RC_CHECK(first.size() == 4 && first[0].seq == 0 && first[3].seq == 3);
  RC_CHECK(s.tick(1).empty());                       // nothing new until something is acknowledged
  s.on_ack(2);                                       // 0 and 1 are done
  const std::vector<DataPacket> more = s.tick(2);
  std::cout << "  window 4, ten messages: first tick sent " << first.size() << "; after acknowledging 2, the next tick sent " << more.size() << " (";
  for (const DataPacket& p : more) std::cout << p.seq << " ";
  std::cout << ")\n\n";
  RC_CHECK(more.size() == 2);
  RC_CHECK(more.size() == 2 && more[0].seq == 4 && more[1].seq == 5);
  RC_CHECK(s.unacknowledged() == 8);
  s.on_ack(10);
  RC_CHECK(s.idle());
}

RC_TEST("a packet with no acknowledgement is sent again after the timeout and not before, and counted") {
  ReliableSender s(2, 10);
  s.send("x");
  RC_CHECK(s.tick(0).size() == 1);
  RC_CHECK(s.tick(9).empty());                       // one tick early
  const std::vector<DataPacket> again = s.tick(10);
  RC_CHECK(again.size() == 1);
  RC_CHECK(s.retransmissions() == 1);
  RC_CHECK(s.tick(19).empty());
  RC_CHECK(s.tick(20).size() == 1);
  RC_CHECK(s.retransmissions() == 2);
  RC_CHECK(s.transmissions() == 3);
  s.on_ack(1);
  RC_CHECK(s.tick(30).empty());                      // acknowledged: nothing more
  RC_CHECK(s.idle());
}

RC_TEST("over a link that loses a fifth of what it carries, sending once loses a fifth of the messages and retransmitting loses none") {
  const int n = 500;
  const Outcome plain = run(n, n, 30, 5, 0.2, false, 3);
  const Outcome reliable = run(n, 8, 30, 5, 0.2, true, 3);
  std::cout << "  500 messages, one way delay 5 ticks, 20 percent loss each way\n"
            << "                          delivered   in order   packets sent per message\n"
            << "  send once               " << std::setw(9) << plain.delivered << "   " << std::setw(8) << (plain.in_order ? "yes" : "no") << "   " << std::fixed
            << std::setprecision(2) << std::setw(10) << static_cast<double>(plain.transmissions) / n << "\n"
            << "  acknowledge and resend  " << std::setw(9) << reliable.delivered << "   " << std::setw(8) << (reliable.in_order ? "yes" : "no") << "   "
            << std::setw(10) << static_cast<double>(reliable.transmissions) / n << "\n\n";
  RC_CHECK(plain.delivered > 0.7 * n && plain.delivered < 0.9 * n);
  RC_CHECK(reliable.delivered == n);
  RC_CHECK(reliable.in_order);
  RC_CHECK(static_cast<double>(reliable.transmissions) / n > 1.2);
  RC_CHECK(static_cast<double>(reliable.transmissions) / n < 2.0);
  RC_CHECK(reliable.retransmissions > 0);
}

RC_TEST("on a link with delay, a window of one is slow, and a wider one delivers the same messages in a fraction of the time") {
  std::cout << "  100 messages, one way delay 50 ticks, no loss, timeout 120 ticks\n  window   ticks to deliver all\n";
  int one = 0;
  int ten = 0;
  int previous = 1 << 30;
  bool falls = true;
  for (int window : {1, 2, 5, 10, 20}) {
    const Outcome o = run(100, window, 120, 50, 0.0, true, 1);
    std::cout << "  " << std::setw(6) << window << "   " << std::setw(20) << o.ticks << "\n";
    RC_CHECK(o.delivered == 100);
    if (o.ticks > previous) falls = false;
    previous = o.ticks;
    if (window == 1) one = o.ticks;
    if (window == 10) ten = o.ticks;
  }
  std::cout << "\n";
  RC_CHECK(falls);
  RC_CHECK(one > 0);
  RC_CHECK(ten * 5 < one);
}
