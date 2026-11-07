#include <rc/test/rc_test.hpp>

#include <iostream>
#include <string>
#include <vector>

#include "solution.hpp"

namespace {

const Nanoseconds kMs = 1000000;
const Nanoseconds kSecond = 1000 * kMs;

}  // namespace

RC_TEST("a one-shot task runs at its time, and the clock reads that time while it runs") {
  TestClock clock;
  Scheduler s(&clock);
  Nanoseconds seen = -1;
  Nanoseconds clock_seen = -1;
  s.at(250 * kMs, [&](Nanoseconds now) {
    seen = now;
    clock_seen = clock.now();
  });

  s.run_until(100 * kMs);
  RC_CHECK_EQ(s.events_run(), 0L);
  RC_CHECK_EQ(clock.now(), 100 * kMs);          // the clock is where the run stopped, and nothing ran

  s.run_until(kSecond);
  RC_CHECK_EQ(s.events_run(), 1L);
  RC_CHECK_EQ(seen, 250 * kMs);
  RC_CHECK_EQ(clock_seen, 250 * kMs);
  RC_CHECK_EQ(clock.now(), kSecond);
}

RC_TEST("a 100 hertz sensor and a 10 hertz logger over ten seconds run exactly as often as they should") {
  TestClock clock;
  Scheduler s(&clock);
  std::vector<std::string> order;
  long sensor = 0;
  long logger = 0;

  s.every(10 * kMs, 0, [&](Nanoseconds now) {
    ++sensor;
    if (now == 100 * kMs) order.push_back("sensor");
  });
  s.every(100 * kMs, 0, [&](Nanoseconds now) {
    ++logger;
    if (now == 100 * kMs) order.push_back("logger");
  });

  s.run_until(10 * kSecond);
  std::cout << "  ten seconds, start and end included: sensor ran " << sensor << " times, logger " << logger
            << " times, " << s.events_run() << " events in all\n";
  RC_CHECK_EQ(sensor, 1001L);
  RC_CHECK_EQ(logger, 101L);
  RC_CHECK_EQ(s.events_run(), 1102L);

  // At 100 ms both are due. The one scheduled first runs first.
  RC_REQUIRE(order.size() == 2);
  RC_CHECK_EQ(order[0], std::string("sensor"));
  RC_CHECK_EQ(order[1], std::string("logger"));
}

RC_TEST("whole nanoseconds land exactly where seconds as doubles do not") {
  // Three ticks of a tenth of a second after the first, within 0.3 seconds: ticks at 0, 0.1, 0.2 and 0.3.
  TestClock clock;
  Scheduler s(&clock);
  long ticks = 0;
  s.every(100 * kMs, 0, [&](Nanoseconds) { ++ticks; });
  s.run_until(300 * kMs);

  // The same plan with times kept as doubles and added up.
  long double_ticks = 0;
  double t = 0.0;
  while (t <= 0.3) {
    ++double_ticks;
    t += 0.1;
  }
  std::cout << "  a tick every 0.1 s up to 0.3 s: " << ticks << " with integer nanoseconds, " << double_ticks
            << " with doubles (the third addition gives " << std::fixed << t - 0.1 << ")\n\n";
  RC_CHECK_EQ(ticks, 4L);
  RC_CHECK_EQ(double_ticks, 3L);
}

RC_TEST("stopping and running on gives the same events as running straight through") {
  long whole = 0;
  {
    TestClock clock;
    Scheduler s(&clock);
    s.every(7 * kMs, 3 * kMs, [&](Nanoseconds) { ++whole; });
    s.run_until(10 * kSecond);
  }

  long piecewise = 0;
  {
    TestClock clock;
    Scheduler s(&clock);
    s.every(7 * kMs, 3 * kMs, [&](Nanoseconds) { ++piecewise; });
    for (int i = 1; i <= 100; ++i) s.run_until(i * 100 * kMs);
    RC_CHECK_EQ(clock.now(), 10 * kSecond);
  }
  std::cout << "  a 7 ms task from 3 ms, ten seconds: " << whole << " events in one run, " << piecewise << " in a hundred\n\n";
  RC_CHECK(whole > 1000);
  RC_CHECK_EQ(whole, piecewise);
}

RC_TEST("a task can schedule another, and it runs in the same run if it is due") {
  TestClock clock;
  Scheduler s(&clock);
  std::vector<Nanoseconds> times;
  s.at(10 * kMs, [&](Nanoseconds now) {
    times.push_back(now);
    s.at(now + 5 * kMs, [&](Nanoseconds later) { times.push_back(later); });
    s.at(now + 500 * kMs, [&](Nanoseconds much_later) { times.push_back(much_later); });
  });
  s.run_until(100 * kMs);
  RC_REQUIRE(times.size() == 2);
  RC_CHECK_EQ(times[0], 10 * kMs);
  RC_CHECK_EQ(times[1], 15 * kMs);
  RC_CHECK_EQ(s.events_run(), 2L);

  s.run_until(kSecond);
  RC_CHECK_EQ(s.events_run(), 3L);
  RC_CHECK_EQ(times.back(), 510 * kMs);
}

RC_TEST("ten minutes of a one kilohertz loop is run without waiting") {
  TestClock clock;
  Scheduler s(&clock);
  Nanoseconds last = 0;
  long late = 0;
  s.every(kMs, 0, [&](Nanoseconds now) {
    if (now < last) ++late;      // time never goes backwards
    last = now;
  });
  s.run_until(600 * kSecond);
  std::cout << "  ten minutes at 1 kHz: " << s.events_run() << " ticks, time went backwards " << late
            << " times, the last tick at " << last / kSecond << " s\n\n";
  RC_CHECK_EQ(s.events_run(), 600001L);
  RC_CHECK_EQ(late, 0L);
  RC_CHECK_EQ(last, 600 * kSecond);
}
