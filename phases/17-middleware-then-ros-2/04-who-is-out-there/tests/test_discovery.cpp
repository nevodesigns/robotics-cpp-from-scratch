#include <rc/test/rc_test.hpp>

#include <cstdint>
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

const char* kind_name(DiscoveryEvent::Kind k) {
  switch (k) {
    case DiscoveryEvent::Kind::appeared: return "appeared";
    case DiscoveryEvent::Kind::restarted: return "restarted";
    case DiscoveryEvent::Kind::gone: return "gone";
  }
  return "?";
}

}  // namespace

RC_TEST("a participant that announces itself appears once, and its repeats are heartbeats and not news") {
  Discovery d(3.0);
  d.announce("lidar", 1, 0.0);
  d.announce("planner", 1, 0.1);
  std::vector<DiscoveryEvent> first = d.poll(0.2);
  RC_CHECK(first.size() == 2);
  RC_CHECK(first.size() == 2 && first[0].kind == DiscoveryEvent::Kind::appeared && first[0].name == "lidar");
  RC_CHECK(first.size() == 2 && first[1].kind == DiscoveryEvent::Kind::appeared && first[1].name == "planner");
  d.announce("lidar", 1, 1.0);
  d.announce("lidar", 1, 2.0);
  RC_CHECK(d.poll(2.1).empty());
  const std::vector<std::string> who = d.peers(2.1);
  RC_CHECK(who.size() == 2);
  RC_CHECK(who.size() == 2 && who[0] == "lidar" && who[1] == "planner");
  RC_CHECK(d.alive("lidar", 2.1));
  RC_CHECK(!d.alive("nobody", 2.1));
}

RC_TEST("a participant that goes quiet for longer than its lease is gone, and said so exactly once") {
  Discovery d(3.0);
  d.announce("lidar", 1, 0.0);
  d.poll(0.0);
  RC_CHECK(d.poll(2.9).empty());                       // 2.9 s of silence is inside the lease
  RC_CHECK(d.alive("lidar", 3.0));                     // exactly the lease is still alive
  const std::vector<DiscoveryEvent> ev = d.poll(3.1);
  std::cout << "  announced at 0, lease 3 s: at 2.9 s no event; at 3.1 s " << ev.size() << " event";
  for (const DiscoveryEvent& e : ev) std::cout << " (" << kind_name(e.kind) << " " << e.name << ")";
  std::cout << "\n\n";
  RC_CHECK(ev.size() == 1);
  RC_CHECK(ev.size() == 1 && ev[0].kind == DiscoveryEvent::Kind::gone && ev[0].name == "lidar");
  RC_CHECK(!d.alive("lidar", 3.1));
  RC_CHECK(d.poll(10.0).empty());                      // not announced gone again
  RC_CHECK(d.peers(10.0).empty());
}

RC_TEST("one that comes back after being gone appears again, and one that restarts without a gap is told apart by its incarnation") {
  Discovery d(2.0);
  d.announce("arm", 1, 0.0);
  d.poll(0.0);
  d.poll(5.0);                                          // gone
  d.announce("arm", 1, 6.0);
  const std::vector<DiscoveryEvent> back = d.poll(6.0);
  RC_CHECK(back.size() == 1);
  RC_CHECK(back.size() == 1 && back[0].kind == DiscoveryEvent::Kind::appeared);

  d.announce("arm", 1, 7.0);                            // a heartbeat
  d.announce("arm", 2, 8.0);                            // the same name, started again before the lease ran out
  const std::vector<DiscoveryEvent> restart = d.poll(8.0);
  std::cout << "  after being gone: " << back.size() << " event (" << kind_name(back[0].kind) << "); a new incarnation inside the lease: "
            << restart.size() << " event (" << (restart.empty() ? "none" : kind_name(restart[0].kind)) << ")\n\n";
  RC_CHECK(restart.size() == 1);
  RC_CHECK(restart.size() == 1 && restart[0].kind == DiscoveryEvent::Kind::restarted);
  d.announce("arm", 2, 9.0);
  RC_CHECK(d.poll(9.0).empty());
}

RC_TEST("a lease is how long a silence takes to mean gone, so a node that dies is noticed after about one lease") {
  std::cout << "  heartbeats every 1 s, the node dies at 10 s, polling every 0.1 s\n  lease (s)   noticed after (s)\n";
  bool tracks = true;
  for (double lease : {1.5, 2.5, 3.5, 5.5}) {
    Discovery d(lease);
    double noticed = -1.0;
    for (int tick = 0; tick <= 300; ++tick) {
      const double now = tick * 0.1;
      if (tick % 10 == 0 && now <= 10.0) d.announce("node", 1, now);
      for (const DiscoveryEvent& e : d.poll(now))
        if (e.kind == DiscoveryEvent::Kind::gone && noticed < 0.0) noticed = now - 10.0;
    }
    std::cout << "  " << std::setw(9) << std::fixed << std::setprecision(1) << lease << "   " << std::setw(17) << std::setprecision(1) << noticed << "\n";
    if (noticed < lease - 0.15 || noticed > lease + 0.25) tracks = false;
  }
  std::cout << "\n";
  RC_CHECK(tracks);
}

RC_TEST("a short lease sees a node die sooner and cries wolf when heartbeats are lost, so the lease is a few heartbeat periods") {
  std::cout << "  heartbeats every 1 s over a link that loses 10 percent of them, 2000 s, the node never dies\n  lease (s)   false 'gone' events\n";
  int false_short = -1;
  int false_long = -1;
  for (double lease : {1.5, 2.5, 3.5, 5.5}) {
    Discovery d(lease);
    Rng rng{17};
    int false_alarms = 0;
    for (int tick = 0; tick <= 20000; ++tick) {
      const double now = tick * 0.1;
      if (tick % 10 == 0 && rng.uniform() >= 0.10) d.announce("node", 1, now);
      if (tick == 0) d.announce("node", 1, now);
      for (const DiscoveryEvent& e : d.poll(now))
        if (e.kind == DiscoveryEvent::Kind::gone) ++false_alarms;
    }
    std::cout << "  " << std::setw(9) << std::fixed << std::setprecision(1) << lease << "   " << std::setw(19) << false_alarms << "\n";
    if (lease == 1.5) false_short = false_alarms;
    if (lease == 5.5) false_long = false_alarms;
  }
  std::cout << "\n";
  RC_CHECK(false_short > 50);
  RC_CHECK(false_long < false_short);
  RC_CHECK(false_long <= 2);
}
