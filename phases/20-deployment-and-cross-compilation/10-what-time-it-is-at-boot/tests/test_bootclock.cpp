#include <rc/test/rc_test.hpp>

#include <cmath>
#include <iomanip>
#include <iostream>

#include "solution.hpp"

namespace {

const double kNow = 1794700000.0;           // about the 12th of November 2026, in seconds since 1970

}  // namespace

RC_TEST("a clock with no battery counts from 1970, says it has not been set, and is corrected once the network gives the time") {
  BootClock c(0.0);
  const Stamp early = c.stamp(12.0);
  RC_CHECK(!early.synced && early.wall == 12.0 && early.mono == 12.0);
  RC_CHECK(!c.synced());
  RC_CHECK(std::isnan(c.wall_of(12.0)));
  c.sync(40.0, kNow);                                    // the network answers 40 s after boot
  const Stamp late = c.stamp(45.0);
  RC_CHECK(c.synced() && late.synced);
  RC_CHECK_NEAR(late.wall, kNow + 5.0, 1e-3);
  RC_CHECK_NEAR(c.wall_of(12.0), kNow - 28.0, 1e-3);     // the early moment, 28 s before the sync, now has a real time
  std::cout << "  boot at wall 1970, a stamp at 12 s read " << early.wall << " unsynced; the time arrives at 40 s; that moment is now known to have been " << std::fixed << std::setprecision(0) << c.wall_of(12.0) << "\n\n";
}

RC_TEST("the wall clock jumps by decades when it is set and by fractions of a second when it is slewed, and the monotonic clock never jumps") {
  BootClock c(0.0);
  const Stamp before = c.stamp(39.9);
  c.sync(40.0, kNow);
  const Stamp after = c.stamp(40.1);
  const double big = jump(before.mono, before.wall, after.mono, after.wall);
  const double slew = jump(100.0, kNow + 60.0, 101.0, kNow + 61.2);
  const double back = jump(100.0, kNow + 60.0, 101.0, kNow - 3600.0 + 61.0);
  std::cout << "  setting the clock: the wall clock jumped " << std::setprecision(0) << big << " s (" << std::setprecision(1) << big / 31557600.0 << " years) while 0.2 s of real time passed\n";
  std::cout << "  a slew: " << std::setprecision(2) << slew << " s; an hour put back: " << std::setprecision(0) << back << " s\n\n";
  RC_CHECK_NEAR(big, kNow - 40.0, 1e-3);
  RC_CHECK(big > 1.7e9);
  RC_CHECK_NEAR(slew, 0.2, 1e-6);
  RC_CHECK_NEAR(back, -3600.0, 1e-6);
  RC_CHECK(classify(big, 1.0) == ClockStep::forward);
  RC_CHECK(classify(slew, 1.0) == ClockStep::none);
  RC_CHECK(classify(back, 1.0) == ClockStep::backward);
  RC_CHECK(classify(-1.0, 1.0) == ClockStep::none && classify(1.0, 1.0) == ClockStep::none);           // on the threshold is not a step
  RC_CHECK_NEAR(jump(5.0, 5.0, 8.0, 8.0), 0.0, 1e-12);
}

RC_TEST("a timeout measured in wall time fires when the clock is set, and one measured in time since boot does not notice") {
  const double timeout = 0.3;
  BootClock c(0.0);
  const Stamp beat = c.stamp(39.9);                      // a heartbeat, just before the network answers
  c.sync(40.0, kNow);
  const Stamp now = c.stamp(40.1);                       // 0.2 s later
  const bool wall_expired = (now.wall - beat.wall) > timeout;
  const bool mono_expired = (now.mono - beat.mono) > timeout;
  std::cout << "  a heartbeat 0.2 s ago, a timeout of 0.3 s, and the clock set in between: wall time says " << (wall_expired ? "EXPIRED (a stop for nothing)" : "fine") << ", time since boot says " << (mono_expired ? "expired" : "fine") << "\n";
  RC_CHECK(wall_expired);
  RC_CHECK(!mono_expired);
  // The other way: the clock is put back, and a timeout in wall time can never fire.
  BootClock d(kNow);
  const Stamp b2 = d.stamp(500.0);
  d.sync(600.0, kNow - 3600.0);
  const Stamp n2 = d.stamp(600.0);
  const bool wall_forever = (n2.wall - b2.wall) > timeout;
  const bool mono_gone = (n2.mono - b2.mono) > timeout;
  std::cout << "  a heartbeat 100 s ago and the clock then put back an hour: wall time says " << (wall_forever ? "expired" : "FINE (negative elapsed time)") << ", time since boot says " << (mono_gone ? "expired" : "fine") << "\n\n";
  RC_CHECK(!wall_forever);
  RC_CHECK(mono_gone);
}

RC_TEST("sorted by the wall clock, the early records of the second boot go before everything the first boot wrote; sorted by boot and time since boot, they do not") {
  std::vector<Record> records;
  // Boot 1: the clock was set at 30 s. Boot 2: the records before the clock was set carry a wall time near 1970.
  records.push_back({1, 31.0, kNow + 1.0, "boot 1: network up"});
  records.push_back({1, 500.0, kNow + 470.0, "boot 1: motor fault"});
  records.push_back({1, 900.0, kNow + 870.0, "boot 1: power lost"});
  records.push_back({2, 3.0, 3.0, "boot 2: starting"});
  records.push_back({2, 9.0, 9.0, "boot 2: drivers up"});
  records.push_back({2, 31.0, kNow + 2000.0, "boot 2: network up"});
  std::vector<Record> by_wall = records;
  std::stable_sort(by_wall.begin(), by_wall.end(), [](const Record& a, const Record& b) { return a.wall < b.wall; });
  const std::vector<Record> right = sort_records(records);
  int misplaced = 0;
  for (std::size_t i = 0; i < records.size(); ++i)
    if (by_wall[i].text != right[i].text) ++misplaced;
  std::cout << "  by wall time:\n";
  for (const Record& r : by_wall) std::cout << "    " << r.text << "\n";
  std::cout << "  " << misplaced << " of 6 in the wrong place\n\n";
  RC_CHECK(right.size() == 6);
  if (right.size() != 6) return;
  RC_CHECK(right[0].text == "boot 1: network up" && right[2].text == "boot 1: power lost" && right[3].text == "boot 2: starting" && right[5].text == "boot 2: network up");
  RC_CHECK(by_wall[0].text == "boot 2: starting");
  RC_CHECK(misplaced == 5);                                          // only the last record is where it belongs
  const std::vector<Record> same = sort_records({{1, 5.0, 0.0, "first"}, {1, 5.0, 0.0, "second"}, {1, 4.0, 0.0, "before"}});
  RC_CHECK(same[0].text == "before" && same[1].text == "first" && same[2].text == "second");
}

RC_TEST("a certificate cannot be judged before the time is known, and is judged properly after") {
  const double not_before = kNow - 86400.0 * 30.0;
  const double not_after = kNow + 86400.0 * 335.0;
  BootClock c(0.0);
  RC_CHECK(check_validity(not_before, not_after, c, 10.0) == Validity::unknown);          // by the clock of 1970 it would be "not yet valid"
  const double naive_now = c.stamp(10.0).wall;
  RC_CHECK(naive_now < not_before);
  c.sync(40.0, kNow);
  RC_CHECK(check_validity(not_before, not_after, c, 50.0) == Validity::valid);
  RC_CHECK(check_validity(kNow + 1000.0, not_after, c, 50.0) == Validity::not_yet);
  RC_CHECK(check_validity(not_before, kNow - 1000.0, c, 50.0) == Validity::expired);
  RC_CHECK(check_validity(kNow + 10.0, not_after, c, 50.0 - 1.0) == Validity::not_yet);
  std::cout << "  before the clock is set the answer is unknown, where the 1970 clock would say 'not yet valid' for a certificate that is fine; after it, valid, not yet and expired are told apart\n\n";
}
