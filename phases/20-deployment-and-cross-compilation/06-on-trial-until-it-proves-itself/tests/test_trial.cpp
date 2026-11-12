#include <rc/test/rc_test.hpp>

#include <iomanip>
#include <iostream>

#include "solution.hpp"

namespace {

// Release 1 is in slot 0 and has run for months: healthy 10 s after it starts and never crashes. Release 2 is in slot 1, and its behaviour is the thing being tried.
RunResult try_release(Behaviour fresh, double soak, bool confirm_at_start, int tries = 3, double deadline = 60.0) {
  Behaviour b[2];
  b[0].healthy_at = 10.0;
  b[1] = fresh;
  BootState s;
  s.active = 0;
  start_trial(s, 1, tries);
  return simulate(s, b, soak, deadline, confirm_at_start, 20);
}

Behaviour release(double crash_at, double healthy_at) {
  Behaviour b;
  b.crash_at = crash_at;
  b.healthy_at = healthy_at;
  return b;
}

void line(const char* what, const RunResult& r) {
  std::cout << "  " << what << std::string(46 - std::string(what).size(), ' ') << std::setw(3) << r.boots << " boots   release " << (r.final_slot == 1 ? "2" : "1") << "   "
            << (r.up ? "up" : "NOT UP") << (r.rolled_back ? "   rolled back" : "") << "\n";
}

}  // namespace

RC_TEST("a release on trial costs a try at each boot, the previous one comes back when they are gone, and confirming ends the trial") {
  BootState s;
  s.active = 0;
  start_trial(s, 1, 3);
  RC_CHECK(s.previous == 0 && s.active == 1 && s.trial && s.tries_left == 3);
  RC_CHECK(on_boot(s) == 1 && s.tries_left == 2);
  RC_CHECK(on_boot(s) == 1 && s.tries_left == 1);
  RC_CHECK(on_boot(s) == 1 && s.tries_left == 0);
  RC_CHECK(s.trial);
  RC_CHECK(on_boot(s) == 0);                             // the fourth boot is the old release
  RC_CHECK(!s.trial && s.rolled_back && s.active == 0);
  RC_CHECK(on_boot(s) == 0);                             // and it stays
  BootState t;
  start_trial(t, 1, 3);
  RC_CHECK(on_boot(t) == 1);
  RC_CHECK(confirm(t));
  RC_CHECK(!t.trial);
  RC_CHECK(!confirm(t));                                 // a second confirmation is nothing
  for (int i = 0; i < 10; ++i) RC_CHECK(on_boot(t) == 1);
  RC_CHECK(!t.rolled_back);
  BootState u;
  RC_CHECK(!confirm(u));
  start_trial(u, 1, 0);                                  // no tries at all: back at the first boot
  RC_CHECK(on_boot(u) == 0 && u.rolled_back);
  start_trial(u, 1, 2);
  RC_CHECK(!u.rolled_back);                              // a new trial forgets the old rollback
}

RC_TEST("a good release confirms itself, and a release that crashes at once is tried three times and then given up on") {
  const RunResult good = try_release(release(-1.0, 20.0), 30.0, false);
  const RunResult crash = try_release(release(2.0, -1.0), 30.0, false);
  const RunResult hang = try_release(release(-1.0, -1.0), 30.0, false);
  std::cout << "  three tries, a deadline of 60 s for the health checks, a soak of 30 s\n";
  line("healthy at 20 s, never crashes", good);
  line("crashes at 2 s", crash);
  line("never becomes healthy, never crashes (hangs)", hang);
  std::cout << "\n";
  RC_CHECK(good.boots == 1 && good.final_slot == 1 && good.up && !good.rolled_back && !good.on_trial);
  RC_CHECK(crash.boots == 4 && crash.final_slot == 0 && crash.up && crash.rolled_back);
  RC_CHECK(hang.boots == 4 && hang.final_slot == 0 && hang.up && hang.rolled_back);
  const RunResult five = try_release(release(2.0, -1.0), 30.0, false, 5);
  RC_CHECK(five.boots == 6);
}

RC_TEST("confirming when the program starts is the same as not checking, and a crash loop is the result") {
  const RunResult early = try_release(release(40.0, 20.0), 30.0, true);
  const RunResult proper = try_release(release(40.0, 20.0), 30.0, false);
  std::cout << "  a release that is healthy at 20 s and crashes at 40 s, every time:\n";
  line("confirmed when the program starts", early);
  line("confirmed after healthy plus a soak of 30 s", proper);
  std::cout << "\n";
  RC_CHECK(!early.up);
  RC_CHECK(early.boots == 20);
  RC_CHECK(early.final_slot == 1 && !early.rolled_back);
  RC_CHECK(proper.up && proper.final_slot == 0 && proper.rolled_back && proper.boots == 4);
}

RC_TEST("a soak shorter than the time a failure takes to show confirms a release that is bad, and the soak is what has to be chosen") {
  std::cout << "  a release that is healthy at 20 s and crashes at 120 s, every time:\n  soak (s)   result\n";
  const double soaks[] = {0.0, 30.0, 90.0, 100.0, 150.0};
  RunResult r[5];
  for (int i = 0; i < 5; ++i) {
    r[i] = try_release(release(120.0, 20.0), soaks[i], false, 3, 60.0);
    std::cout << "  " << std::setw(8) << std::fixed << std::setprecision(0) << soaks[i] << "   " << (r[i].up ? "stays up" : "crash loop") << ", release " << (r[i].final_slot == 1 ? "2" : "1") << ", " << r[i].boots << " boots\n";
  }
  std::cout << "\n";
  RC_CHECK(!r[0].up && r[0].boots == 20);                // confirmed at 20 s, crashes at 120: nothing to roll back to
  RC_CHECK(!r[1].up);
  RC_CHECK(!r[2].up);                                    // 20 + 90 = 110, before the crash
  RC_CHECK(r[3].up && r[3].rolled_back);                 // 20 + 100 = 120: the crash comes first (the confirmation at the same instant does not count)
  RC_CHECK(r[4].up && r[4].final_slot == 0 && r[4].boots == 4);
}

RC_TEST("a release whose checks pass after the deadline is treated as one that never passed") {
  const RunResult late = try_release(release(-1.0, 90.0), 10.0, false);
  const RunResult in_time = try_release(release(-1.0, 50.0), 5.0, false);
  std::cout << "  healthy at 90 s with a deadline of 60 s: " << late.boots << " boots, release " << (late.final_slot == 1 ? "2" : "1") << (late.rolled_back ? ", rolled back" : "") << "; healthy at 50 s: " << in_time.boots << " boot, release "
            << (in_time.final_slot == 1 ? "2" : "1") << "\n\n";
  RC_CHECK(late.rolled_back && late.final_slot == 0 && late.boots == 4);
  RC_CHECK(in_time.boots == 1 && in_time.final_slot == 1 && !in_time.on_trial);
}
