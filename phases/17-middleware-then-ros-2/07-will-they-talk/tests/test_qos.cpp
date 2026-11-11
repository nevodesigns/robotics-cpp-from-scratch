#include <rc/test/rc_test.hpp>

#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

#include "solution.hpp"

namespace {

const char* name_of(Reliability r) { return r == Reliability::reliable ? "reliable" : "best effort"; }

}  // namespace

RC_TEST("reliability: a reliable offer meets either request, and a best effort offer meets only a best effort one") {
  std::cout << "  reliability   offered \\ requested   best effort   reliable\n";
  int fits = 0;
  for (Reliability offered : {Reliability::best_effort, Reliability::reliable}) {
    std::cout << "                " << std::left << std::setw(20) << name_of(offered) << std::right;
    for (Reliability requested : {Reliability::best_effort, Reliability::reliable}) {
      Qos o;
      o.reliability = offered;
      Qos r;
      r.reliability = requested;
      const bool ok = check(o, r).compatible;
      if (ok) ++fits;
      std::cout << std::setw(14) << (ok ? "talk" : "SILENT");
    }
    std::cout << "\n";
  }
  std::cout << "\n";
  Qos best;
  best.reliability = Reliability::best_effort;
  Qos rel;
  rel.reliability = Reliability::reliable;
  RC_CHECK(check(best, best).compatible);
  RC_CHECK(check(rel, best).compatible);
  RC_CHECK(check(rel, rel).compatible);
  RC_CHECK(!check(best, rel).compatible);
  RC_CHECK(fits == 3);
}

RC_TEST("durability: a transient local offer meets either request, and a volatile offer cannot give a late joiner what it asks for") {
  Qos vol;
  Qos tl;
  tl.durability = Durability::transient_local;
  RC_CHECK(check(vol, vol).compatible);
  RC_CHECK(check(tl, vol).compatible);
  RC_CHECK(check(tl, tl).compatible);
  const Compatibility bad = check(vol, tl);
  std::cout << "  volatile offered, transient local requested: " << (bad.compatible ? "talk" : "SILENT") << " (" << (bad.reasons.empty() ? "" : bad.reasons[0]) << ")\n\n";
  RC_CHECK(!bad.compatible);
  RC_CHECK(bad.reasons.size() == 1);
}

RC_TEST("deadline and lease: the offer must be at least as quick as the request, and saying nothing cannot meet a demand") {
  Qos none;
  Qos fast;
  fast.deadline = 0.1;
  Qos slow;
  slow.deadline = 0.5;
  RC_CHECK(check(fast, slow).compatible);              // offers a message every 0.1 s, asked for one every 0.5 s
  RC_CHECK(!check(slow, fast).compatible);             // offers one every 0.5 s, asked for every 0.1 s
  RC_CHECK(check(none, none).compatible);              // nothing asked, nothing offered
  RC_CHECK(check(fast, none).compatible);              // nothing asked
  RC_CHECK(!check(none, fast).compatible);             // something asked, nothing offered

  Qos short_lease;
  short_lease.lease = 1.0;
  Qos long_lease;
  long_lease.lease = 5.0;
  RC_CHECK(check(short_lease, long_lease).compatible);
  RC_CHECK(!check(long_lease, short_lease).compatible);
  RC_CHECK(!check(none, short_lease).compatible);

  Qos manual;
  manual.liveliness = Liveliness::manual_by_topic;
  RC_CHECK(check(manual, none).compatible);
  RC_CHECK(!check(none, manual).compatible);
}

RC_TEST("every property that does not fit is named, so a mismatch can be explained and not just detected") {
  Qos offered;
  offered.reliability = Reliability::best_effort;
  offered.deadline = 1.0;
  Qos requested;
  requested.reliability = Reliability::reliable;
  requested.durability = Durability::transient_local;
  requested.deadline = 0.2;
  const Compatibility c = check(offered, requested);
  std::cout << "  a camera driver offering best effort at 1 s, a node requesting reliable, transient local, 0.2 s: " << c.reasons.size() << " reasons\n";
  for (const std::string& r : c.reasons) std::cout << "    " << r << "\n";
  std::cout << "\n";
  RC_CHECK(!c.compatible);
  RC_CHECK(c.reasons.size() == 3);
  RC_CHECK(check(Qos(), Qos()).reasons.empty());
}

RC_TEST("a camera driver in sensor data profile and a node that subscribes with the default profile are never connected") {
  const std::vector<Endpoint> pubs = {Endpoint{"camera", "image", sensor_data_profile()}, Endpoint{"map_server", "map", latched_profile()}};
  const std::vector<Endpoint> subs = {
      Endpoint{"detector", "image", default_profile()},         // reliable
      Endpoint{"viewer", "image", sensor_data_profile()},       // best effort
      Endpoint{"planner", "map", default_profile()},
      Endpoint{"late_planner", "map", latched_profile()},
      Endpoint{"logger", "scan", default_profile()},            // nobody publishes it
  };
  const MatchResult r = match(pubs, subs);
  std::cout << "  links: ";
  for (const Link& l : r.links) std::cout << l.publisher << "->" << l.subscriber << " ";
  std::cout << "\n  not connected: ";
  for (const Mismatch& m : r.mismatches) std::cout << m.publisher << "->" << m.subscriber << " (" << m.reasons.size() << " reason) ";
  std::cout << "\n\n";
  RC_CHECK(r.links.size() == 3);
  RC_CHECK(r.links.size() == 3 && r.links[0].publisher == "camera" && r.links[0].subscriber == "viewer");
  RC_CHECK(r.links.size() == 3 && r.links[1].subscriber == "planner");
  RC_CHECK(r.links.size() == 3 && r.links[2].subscriber == "late_planner");
  RC_CHECK(r.mismatches.size() == 1);
  RC_CHECK(r.mismatches.size() == 1 && r.mismatches[0].subscriber == "detector");
  RC_CHECK(r.mismatches.size() == 1 && r.mismatches[0].reasons.size() == 1);
}
