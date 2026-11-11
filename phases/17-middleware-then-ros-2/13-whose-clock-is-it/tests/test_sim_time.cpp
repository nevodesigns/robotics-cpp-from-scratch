#include <rc/test/rc_test.hpp>

#include <cmath>
#include <iomanip>
#include <iostream>

#include "solution.hpp"

RC_TEST("a timer fires once for every period of the clock's time that goes by, and not at all while the clock stands still") {
  SimClock clock;
  SimTimer timer(0.5, 1);
  RC_CHECK(timer.poll(clock) == 0);                      // the first poll arms it
  int fired = 0;
  for (int i = 1; i <= 100; ++i) {
    clock.on_clock(i * 0.1);                             // ten seconds in steps of a tenth
    fired += timer.poll(clock);
  }
  RC_CHECK(fired == 20);
  // The simulation is paused: the same time arrives again and again.
  int while_paused = 0;
  for (int i = 0; i < 1000; ++i) {
    clock.on_clock(10.0);
    while_paused += timer.poll(clock);
  }
  std::cout << "  period 0.5 s over 10 s of the clock: " << fired << " firings; while the clock stood still for 1000 polls: " << while_paused << "\n\n";
  RC_CHECK(while_paused == 0);
}

RC_TEST("firings per second of the clock do not depend on how fast the clock runs, firings per second of the wall do") {
  std::cout << "  period 0.5 s, 10 wall seconds of 1000 updates\n  clock speed   clock seconds   firings   per clock second   per wall second\n";
  bool constant = true;
  int slow = 0;
  int fast = 0;
  for (double speed : {0.5, 1.0, 2.0, 10.0}) {
    SimClock clock;
    SimTimer timer(0.5, 1000);
    timer.poll(clock);
    int fired = 0;
    for (int i = 1; i <= 1000; ++i) {
      clock.on_clock(i * 0.01 * speed);                  // one update every 10 ms of the wall
      fired += timer.poll(clock);
    }
    const double clock_seconds = clock.now();
    std::cout << "  " << std::setw(11) << std::fixed << std::setprecision(1) << speed << "   " << std::setw(13) << clock_seconds << "   " << std::setw(7) << fired << "   "
              << std::setw(16) << std::setprecision(2) << fired / clock_seconds << "   " << std::setw(15) << fired / 10.0 << "\n";
    if (std::fabs(fired / clock_seconds - 2.0) > 0.05) constant = false;
    if (speed == 0.5) slow = fired;
    if (speed == 10.0) fast = fired;
  }
  std::cout << "\n";
  RC_CHECK(constant);
  RC_CHECK(slow > 0);
  RC_CHECK(fast == 20 * slow);
}

RC_TEST("a clock that jumps forward over many periods fires a bounded number of times, and says how many it skipped") {
  SimClock clock;
  SimTimer limited(0.5, 1);
  SimTimer storm(0.5, 100000);                           // a timer that makes up every one it missed
  limited.poll(clock);
  storm.poll(clock);
  clock.on_clock(100.0);                                 // the simulator jumps 100 seconds ahead
  const int a = limited.poll(clock);
  const int b = storm.poll(clock);
  std::cout << "  a jump of 100 s over a period of 0.5 s (200 periods): a timer allowed to catch up once fired " << a << " and skipped " << limited.skipped()
            << "; one that makes up every period fired " << b << " times at once\n\n";
  RC_CHECK(a == 1);
  RC_CHECK(limited.skipped() == 199);
  RC_CHECK(b == 200);
  clock.on_clock(100.6);
  RC_CHECK(limited.poll(clock) == 1);                    // and carries on from there: the next is at 100.5
  RC_CHECK(limited.poll(clock) == 0);
}

RC_TEST("a clock that jumps back starts the timer again, where one that kept its old schedule is silent until the clock catches up") {
  SimClock clock;
  SimTimer timer(0.5, 1);
  timer.poll(clock);
  // A timer that does not look at the epoch: it keeps its schedule and fires when the time reaches it.
  double naive_next = 0.5;
  int naive_before = 0;
  int before = 0;
  for (int i = 1; i <= 1000; ++i) {
    clock.on_clock(i * 0.1);                             // up to 100 s
    before += timer.poll(clock);
    while (clock.now() >= naive_next) {
      ++naive_before;
      naive_next += 0.5;
    }
  }
  RC_CHECK(clock.jumps_back() == 0);
  clock.on_clock(0.0);                                   // the simulation restarts
  RC_CHECK(clock.jumps_back() == 1);
  RC_CHECK(clock.epoch() == 1);
  int after = 0;
  int naive_after = 0;
  for (int i = 1; i <= 1000; ++i) {
    clock.on_clock(i * 0.1);                             // 100 seconds of the new run
    after += timer.poll(clock);
    if (clock.now() >= naive_next && naive_next <= clock.now()) {
      ++naive_after;
      naive_next += 0.5;
    }
  }
  std::cout << "  100 s of the clock, then a restart to 0 and 100 s more\n  fired before the restart: " << before << ", after: " << after << " (one lost to re-arming)\n"
            << "  a timer that kept its old schedule: fired " << naive_before << " before, " << naive_after << " in the first 100 s after\n\n";
  RC_CHECK(before == 200);
  RC_CHECK(after == 199);
  RC_CHECK(naive_before == 200);
  RC_CHECK(naive_after == 0);
}

RC_TEST("a time that goes forward in small steps, the same time twice and the first time ever are not jumps") {
  SimClock clock;
  clock.on_clock(1.0);
  clock.on_clock(1.0);
  clock.on_clock(1.5);
  clock.on_clock(1.5);
  RC_CHECK(clock.jumps_back() == 0);
  RC_CHECK(clock.epoch() == 0);
  RC_CHECK_NEAR(clock.now(), 1.5, 1e-12);
  clock.on_clock(1.4999);
  RC_CHECK(clock.jumps_back() == 1);                     // any step back counts, however small
  RC_CHECK_NEAR(clock.now(), 1.4999, 1e-12);
}
