#include <rc/test/rc_test.hpp>

#include <cmath>
#include <iomanip>
#include <iostream>
#include <type_traits>
#include <utility>

#include "solution.hpp"

namespace {

enum Code { kSpeed = 1, kRange = 2, kStop = 3 };

static_assert(noexcept(std::declval<BlackBox&>().record(0.0, 0, 0.0)), "recording must not throw");

const char* fault_name(Fault f) { return f == Fault::none ? "none" : (f == Fault::gap ? "gap" : "altered"); }

}  // namespace

RC_TEST("events are kept in order with a sequence number and a chain, and an untouched log verifies") {
  BlackBox box(16);
  for (int i = 0; i < 5; ++i) box.record(0.1 * i, kSpeed, 1.0 + i);
  const Dump d = box.dump();
  RC_CHECK(box.size() == 5);
  RC_CHECK(box.total() == 5);
  RC_CHECK(d.dropped == 0);
  RC_CHECK(d.base_chain == 0);
  RC_CHECK(d.events.size() == 5);
  if (d.events.size() != 5) return;
  for (int i = 0; i < 5; ++i) {
    RC_CHECK(d.events[static_cast<std::size_t>(i)].seq == static_cast<std::uint64_t>(i));
    RC_CHECK(d.events[static_cast<std::size_t>(i)].chain != 0);
  }
  RC_CHECK(d.events[0].chain != d.events[1].chain);
  RC_CHECK(verify(d).ok);
  RC_CHECK(verify(BlackBox(4).dump()).ok);                      // an empty log is fine
  BlackBox same(16);
  for (int i = 0; i < 5; ++i) same.record(0.1 * i, kSpeed, 1.0 + i);
  RC_CHECK(same.dump().events[4].chain == d.events[4].chain);   // the same events give the same chain
  BlackBox other(16);
  for (int i = 0; i < 5; ++i) other.record(0.1 * i, kSpeed, i == 2 ? 9.0 : 1.0 + i);
  RC_CHECK(other.dump().events[4].chain != d.events[4].chain);  // a change in one shows at the end
  std::cout << "  five events: sequence 0 to 4, an untouched log verifies, and one different value in the middle changes every chain value after it\n\n";
}

RC_TEST("when the ring is full the oldest event goes, and what is left still verifies because the chain starts from the last one dropped") {
  BlackBox box(8);
  for (int i = 0; i < 20; ++i) box.record(0.1 * i, kSpeed, i);
  const Dump d = box.dump();
  RC_CHECK(box.size() == 8);
  RC_CHECK(box.capacity() == 8);
  RC_CHECK(box.total() == 20);
  RC_CHECK(d.dropped == 12);
  RC_CHECK(d.events.size() == 8);
  if (d.events.size() != 8) return;
  RC_CHECK(d.events.front().seq == 12);
  RC_CHECK(d.events.back().seq == 19);
  RC_CHECK(d.base_chain != 0);
  const Verified v = verify(d);
  // The check a first attempt makes: start the chain from nothing.
  Dump naive = d;
  naive.base_chain = 0;
  const Verified n = verify(naive);
  std::cout << "  20 events into a ring of 8: " << d.dropped << " dropped, sequence " << d.events.front().seq << " to " << d.events.back().seq << "; verifies from the kept link: " << (v.ok ? "yes" : "NO") << "; from zero: " << (n.ok ? "yes" : "no, it calls the first event ") << fault_name(n.fault) << "\n\n";
  RC_CHECK(v.ok);
  RC_CHECK(!n.ok);
  RC_CHECK(n.index == 0);
  RC_CHECK(n.fault == Fault::altered);
  for (int i = 20; i < 100; ++i) box.record(0.1 * i, kSpeed, i);
  RC_CHECK(verify(box.dump()).ok);                              // and after many wraps
  RC_CHECK(box.dump().dropped == 92);
}

RC_TEST("an altered event, a deleted one and two swapped are each found, and the newest events cut off are not") {
  BlackBox box(32);
  for (int i = 0; i < 20; ++i) box.record(0.1 * i, kSpeed, i);
  const Dump good = box.dump();
  RC_CHECK(good.events.size() == 20);
  if (good.events.size() != 20) return;
  Dump altered = good;
  altered.events[7].value = 99.0;
  Dump deleted = good;
  deleted.events.erase(deleted.events.begin() + 7);
  Dump swapped = good;
  std::swap(swapped.events[7], swapped.events[8]);
  Dump truncated = good;
  truncated.events.resize(15);
  const Verified a = verify(altered);
  const Verified d = verify(deleted);
  const Verified s = verify(swapped);
  const Verified t = verify(truncated);
  std::cout << "  twenty events, then:\n  value of event 7 changed      -> " << fault_name(a.fault) << " at " << a.index << "\n  event 7 deleted               -> " << fault_name(d.fault) << " at " << d.index
            << "\n  events 7 and 8 swapped        -> " << fault_name(s.fault) << " at " << s.index << "\n  the last five cut off         -> " << (t.ok ? "not noticed" : "noticed") << "\n\n";
  RC_CHECK(!a.ok && a.fault == Fault::altered && a.index == 7);
  RC_CHECK(!d.ok && d.fault == Fault::gap && d.index == 7);
  RC_CHECK(!s.ok && s.fault == Fault::gap && s.index == 7);
  RC_CHECK(t.ok);                                              // the end of a chain has nothing after it to disagree
}

RC_TEST("the events before a stop show how it came about, even after the ring has wrapped many times") {
  BlackBox box(64);
  double x = 0.0;
  double t = 0.0;
  int ticks = 0;
  while (true) {
    const double range = 18.0 - x;
    box.record(t, kSpeed, 1.5);
    box.record(t, kRange, range);
    if (range < 0.6) {
      box.record(t, kStop, 4.0);                               // a stop, with its cause bits (fence, bit 4)
      break;
    }
    x += 1.5 * 0.1;
    t += 0.1;
    ++ticks;
  }
  const Dump d = box.dump();
  const std::vector<Event> before = window_before(d, kStop, 6);
  std::cout << "  driving at 1.5 m/s at a wall 18 m away, two events a tick: " << box.total() << " recorded, " << d.dropped << " overwritten, the ring holds the last " << d.events.size() << "\n";
  std::cout << "  the six events before the stop:\n";
  for (const Event& e : before) std::cout << "    t " << std::fixed << std::setprecision(1) << e.time << " s  " << (e.code == kSpeed ? "speed" : "range") << " " << std::setprecision(2) << e.value << "\n";
  std::cout << "\n";
  RC_CHECK(verify(d).ok);
  RC_CHECK(d.dropped > 100);
  RC_CHECK(before.size() == 6);
  if (before.size() != 6) return;
  RC_CHECK(before[1].code == kRange && before[3].code == kRange && before[5].code == kRange);
  RC_CHECK(before[1].value > before[3].value && before[3].value > before[5].value);   // the range was closing
  RC_CHECK(before[5].value < 0.8);
  RC_CHECK(window_before(d, 99, 3).empty());                  // no such event
  RC_CHECK(window_before(d, kStop, 1000).size() == d.events.size() - 1);   // fewer than asked for: all there is
  RC_CHECK(ticks > 100);
}

RC_TEST("a clock that went back is counted and the order is kept by the sequence number, and a box of no size is harmless") {
  BlackBox box(8);
  box.record(5.0, kSpeed, 1.0);
  box.record(4.0, kSpeed, 2.0);                               // the clock was set back
  box.record(6.0, kSpeed, 3.0);
  const Dump d = box.dump();
  const Verified v = verify(d);
  RC_CHECK(d.events.size() == 3);
  if (d.events.size() != 3) return;
  std::cout << "  times 5, 4, 6: verified " << (v.ok ? "yes" : "no") << ", " << v.time_regressions << " time going back; the order is by sequence: " << d.events[0].seq << ", " << d.events[1].seq << ", " << d.events[2].seq << "\n\n";
  RC_CHECK(v.ok);
  RC_CHECK(v.time_regressions == 1);
  RC_CHECK(d.events.size() == 3);
  BlackBox none(0);
  none.record(1.0, kSpeed, 1.0);
  RC_CHECK(none.size() == 0);
  RC_CHECK(none.dump().events.empty());
  RC_CHECK(verify(none.dump()).ok);
}
