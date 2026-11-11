#include <rc/test/rc_test.hpp>

#include <cstdint>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

#include "solution.hpp"

namespace {

Bag sample_bag() {
  Bag b;
  b.add("scan", 100.0, std::string("one", 3));
  b.add("pose", 100.1, std::string("\0\x01\xFF binary", 10));      // a payload with a zero byte in it
  b.add("scan", 100.3, "three");
  b.add("scan", 101.0, "");
  return b;
}

}  // namespace

RC_TEST("a bag written to bytes and read back is the same bag, stamps to the last bit and payloads with zero bytes in them") {
  const Bag original = sample_bag();
  const std::vector<std::uint8_t> bytes = original.serialize();
  Bag back;
  const BagStatus s = Bag::parse(bytes, back);
  std::cout << "  " << original.entries().size() << " messages written as " << bytes.size() << " bytes, read back " << back.entries().size() << ": status "
            << (s == BagStatus::ok ? "ok" : "not ok") << "\n\n";
  RC_CHECK(s == BagStatus::ok);
  RC_CHECK(back.entries().size() == 4);
  if (back.entries().size() != 4) return;
  for (std::size_t i = 0; i < 4; ++i) {
    RC_CHECK(back.entries()[i].topic == original.entries()[i].topic);
    RC_CHECK(back.entries()[i].stamp == original.entries()[i].stamp);
    RC_CHECK(back.entries()[i].payload == original.entries()[i].payload);
  }
  RC_CHECK(back.entries()[1].payload.size() == 10);
  RC_CHECK_NEAR(back.duration(), 1.0, 1e-12);
  RC_CHECK(back.count("scan") == 3);
  RC_CHECK(back.count("pose") == 1);
  RC_CHECK(bytes.size() > 6 && bytes[0] == 'R' && bytes[5] == '1');
}

RC_TEST("a file that is not a bag, or that stops in the middle, is refused at every point it could stop, and leaves nothing behind") {
  const std::vector<std::uint8_t> good = sample_bag().serialize();
  int accepted_short = 0;
  int refused_short = 0;
  BagStatus last_short = BagStatus::ok;
  for (std::size_t len = 0; len < good.size(); ++len) {
    const std::vector<std::uint8_t> cut(good.begin(), good.begin() + static_cast<std::ptrdiff_t>(len));
    Bag b;
    b.add("leftover", 0.0, "x");
    const BagStatus s = Bag::parse(cut, b);
    if (s == BagStatus::ok) ++accepted_short;
    else ++refused_short;
    if (s != BagStatus::ok && !b.entries().empty()) ++accepted_short;     // a failure must not leave entries
    last_short = s;
  }
  std::vector<std::uint8_t> wrong = good;
  wrong[0] = 'X';
  Bag w;
  const BagStatus magic = Bag::parse(wrong, w);
  std::cout << "  every prefix of the file, " << good.size() << " of them: " << refused_short << " refused, " << accepted_short << " accepted or left something behind; a file with the wrong first letter: "
            << (magic == BagStatus::bad_magic ? "bad_magic" : "other") << "\n\n";
  RC_CHECK(accepted_short == 0);
  RC_CHECK(refused_short == static_cast<int>(good.size()));
  RC_CHECK(last_short != BagStatus::ok);
  RC_CHECK(magic == BagStatus::bad_magic);
  Bag ok;
  RC_CHECK(Bag::parse(good, ok) == BagStatus::ok);
}

RC_TEST("a count that claims more messages than the file could hold is refused before anything is allocated for them") {
  ByteWriter w;
  const char magic[] = {'R', 'C', 'B', 'A', 'G', '1'};
  for (char c : magic) w.put_u8(static_cast<std::uint8_t>(c));
  w.put_u32(4000000000u);                                 // four billion messages
  w.put_string("scan");
  w.put_f64(1.0);
  w.put_string("x");
  Bag b;
  const BagStatus s = Bag::parse(w.bytes(), b);
  std::cout << "  a header claiming 4000000000 messages in a file of " << w.bytes().size() << " bytes: " << (s == BagStatus::too_many ? "too_many" : "other") << "\n\n";
  RC_CHECK(s == BagStatus::too_many);
  RC_CHECK(b.entries().empty());
}

RC_TEST("a player puts messages back at the pace they were made, none early and none twice, counted from the first") {
  const Bag bag = sample_bag();               // at 0, 0.1, 0.3 and 1.0 seconds after the first
  TopicBus bus;
  std::vector<std::string> topics;
  bus.subscribe("scan", [&](const std::string& t, const std::string&) { topics.push_back(t); });
  bus.subscribe("pose", [&](const std::string& t, const std::string&) { topics.push_back(t); });
  Player player(bag, 1.0);
  std::cout << "  steps of 0.05 s: published per step";
  int total = 0;
  std::vector<int> per_step;
  for (int i = 0; i < 24; ++i) {
    const int n = player.step(0.05, bus);
    per_step.push_back(n);
    total += n;
    std::cout << " " << n;
  }
  std::cout << "\n\n";
  RC_CHECK(total == 4);
  RC_CHECK(per_step.size() == 24);
  if (per_step.size() != 24) return;
  RC_CHECK(per_step[1] == 1);                // by 0.10 s the pose is due (the first, at 0, was due at the first step)
  RC_CHECK(per_step[0] == 1);
  RC_CHECK(per_step[5] == 1);                // 0.30 s
  RC_CHECK(per_step[19] == 1);               // 1.00 s
  RC_CHECK(player.finished());
  RC_CHECK(topics.size() == 4);
  RC_CHECK(topics.size() == 4 && topics[0] == "scan" && topics[1] == "pose" && topics[2] == "scan" && topics[3] == "scan");
  RC_CHECK(player.step(1.0, bus) == 0);
}

RC_TEST("at twice the rate the same messages come in half the time, and at half the rate in twice") {
  std::cout << "  rate   seconds until the last message is published\n";
  double at_1 = 0.0;
  double at_2 = 0.0;
  double at_half = 0.0;
  for (double rate : {0.5, 1.0, 2.0, 4.0}) {
    TopicBus bus;
    bus.subscribe("scan", [](const std::string&, const std::string&) {});
    Player player(sample_bag(), rate);
    double t = 0.0;
    while (!player.finished() && t < 100.0) {
      player.step(0.01, bus);
      t += 0.01;
    }
    std::cout << "  " << std::setw(4) << std::fixed << std::setprecision(1) << rate << "   " << std::setw(10) << std::setprecision(2) << t << "\n";
    if (rate == 1.0) at_1 = t;
    if (rate == 2.0) at_2 = t;
    if (rate == 0.5) at_half = t;
  }
  std::cout << "\n";
  RC_CHECK(at_1 > 0.9);
  RC_CHECK(at_2 > 0.45 && at_2 < 0.56);
  RC_CHECK(at_half > 1.9 && at_half < 2.1);
}

RC_TEST("a node fed from a recording gives exactly the output it gave live, so a bad day can be replayed as often as it takes to understand it") {
  // The node: a running average of what it is sent on "scan", published on "average". Its state lives in the caller, so that it outlives the setup.
  struct Node {
    double sum = 0.0;
    int count = 0;
  };
  const auto run_node = [](TopicBus& bus, Node& node, std::vector<std::string>& outputs) {
    bus.subscribe("scan", [&bus, &node](const std::string&, const std::string& payload) {
      node.sum += static_cast<double>(payload.size());
      ++node.count;
      bus.publish("average", std::to_string(node.sum / node.count));
    });
    bus.subscribe("average", [&outputs](const std::string&, const std::string& payload) { outputs.push_back(payload); });
  };

  // The live run, recorded as it goes.
  Bag recorded;
  std::vector<std::string> live;
  {
    TopicBus bus;
    Node node;
    run_node(bus, node, live);
    bus.subscribe("scan", [&recorded](const std::string& t, const std::string& p) { recorded.add(t, 0.0, p); });
    const char* scans[] = {"aa", "bbbb", "c", "dddddd", "ee"};
    for (const char* s : scans) bus.publish("scan", s);
  }
  // The same bytes, from a file, into a new bus.
  Bag from_file;
  RC_CHECK(Bag::parse(recorded.serialize(), from_file) == BagStatus::ok);
  std::vector<std::string> replayed;
  {
    TopicBus bus;
    Node node;
    run_node(bus, node, replayed);
    Player player(from_file, 1.0);
    while (!player.finished()) player.step(0.1, bus);
  }
  std::cout << "  live output:     ";
  for (const std::string& s : live) std::cout << s.substr(0, 5) << " ";
  std::cout << "\n  replayed output: ";
  for (const std::string& s : replayed) std::cout << s.substr(0, 5) << " ";
  std::cout << "\n\n";
  RC_CHECK(live.size() == 5);
  RC_CHECK(replayed == live);
}
