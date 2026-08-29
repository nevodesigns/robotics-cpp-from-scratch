#include <rc/test/rc_test.hpp>

#include <memory>

#include "solution.hpp"

namespace {
constexpr Nanoseconds kMillisecond = 1000000;
constexpr Nanoseconds kSecond = 1000 * kMillisecond;
}  // namespace

RC_TEST("a test clock starts where it was told to") {
  const TestClock clock(500);
  RC_CHECK_EQ(clock.now(), Nanoseconds{500});
}

RC_TEST("a test clock advances by exactly what it is given") {
  TestClock clock(0);
  clock.advance(kSecond);
  RC_CHECK_EQ(clock.now(), kSecond);
  clock.advance(kSecond);
  RC_CHECK_EQ(clock.now(), 2 * kSecond);
}

RC_TEST("a test clock can be put anywhere, including backwards") {
  // Arranging a backwards step on demand is the reason the clock is injected.
  // With the real one this case could not be tested at all.
  TestClock clock(10 * kSecond);
  clock.set(kSecond);
  RC_CHECK_EQ(clock.now(), kSecond);
}

RC_TEST("an interval converts to seconds") {
  RC_CHECK_NEAR(seconds_between(0, kSecond), 1.0, 1e-12);
  RC_CHECK_NEAR(seconds_between(0, 250 * kMillisecond), 0.25, 1e-12);
  RC_CHECK_NEAR(seconds_between(kSecond, kSecond), 0.0, 1e-12);
}

RC_TEST("an interval keeps sub millisecond precision") {
  RC_CHECK_NEAR(seconds_between(0, 1500), 0.0000015, 1e-15);
}

RC_TEST("a backwards interval is negative rather than enormous") {
  // Computed with unsigned arithmetic this would wrap to a vast positive
  // number, which is how a clock step becomes a lurch instead of a warning.
  RC_CHECK(seconds_between(2 * kSecond, kSecond) < 0.0);
  RC_CHECK_NEAR(seconds_between(2 * kSecond, kSecond), -1.0, 1e-12);
}

RC_TEST("the first tick has no interval to report") {
  TestClock clock(5 * kSecond);
  LoopTimer timer(0.1);

  const TickResult first = timer.tick(clock);
  RC_CHECK(first.quality == TickQuality::First);
  RC_CHECK_NEAR(first.dt, 0.0, 1e-12);
}

RC_TEST("the first tick does not report the time since the clock's origin") {
  // A clock reading five seconds does not mean five seconds have elapsed in
  // this loop. Reporting that would make a controller integrate a value that
  // has nothing to do with it.
  TestClock clock(5 * kSecond);
  LoopTimer timer(0.1);
  RC_CHECK_NEAR(timer.tick(clock).dt, 0.0, 1e-12);
}

RC_TEST("an ordinary tick reports the interval") {
  TestClock clock(0);
  LoopTimer timer(0.1);
  timer.tick(clock);

  clock.advance(10 * kMillisecond);
  const TickResult second = timer.tick(clock);
  RC_CHECK(second.quality == TickQuality::Good);
  RC_CHECK_NEAR(second.dt, 0.01, 1e-12);
}

RC_TEST("intervals are measured tick to tick, not from the start") {
  TestClock clock(0);
  LoopTimer timer(1.0);
  timer.tick(clock);

  clock.advance(10 * kMillisecond);
  RC_CHECK_NEAR(timer.tick(clock).dt, 0.01, 1e-12);

  clock.advance(20 * kMillisecond);
  RC_CHECK_NEAR(timer.tick(clock).dt, 0.02, 1e-12);
}

RC_TEST("a clock that steps backwards is reported, not integrated") {
  // The scheduled bug this lesson exists for. A controller handed a negative
  // interval does something sudden, so the timer refuses to hand one over.
  TestClock clock(10 * kSecond);
  LoopTimer timer(0.1);
  timer.tick(clock);

  clock.set(9 * kSecond);
  const TickResult stepped = timer.tick(clock);
  RC_CHECK(stepped.quality == TickQuality::Backwards);
  RC_CHECK_NEAR(stepped.dt, 0.0, 1e-12);
  RC_CHECK(!stepped.usable());
}

RC_TEST("two ticks with no time between them are not usable either") {
  TestClock clock(kSecond);
  LoopTimer timer(0.1);
  timer.tick(clock);

  const TickResult same = timer.tick(clock);
  RC_CHECK(same.quality == TickQuality::Backwards);
  RC_CHECK(!same.usable());
}

RC_TEST("a stall is clamped and reported") {
  TestClock clock(0);
  LoopTimer timer(0.1);
  timer.tick(clock);

  clock.advance(3 * kSecond);   // the loop was descheduled
  const TickResult stalled = timer.tick(clock);
  RC_CHECK(stalled.quality == TickQuality::Stalled);
  RC_CHECK_NEAR(stalled.dt, 0.1, 1e-12);

  // Clamped is still usable: the loop should keep running, it just must not
  // integrate three seconds in one step.
  RC_CHECK(stalled.usable());
}

RC_TEST("the timer recovers after a stall") {
  TestClock clock(0);
  LoopTimer timer(0.1);
  timer.tick(clock);

  clock.advance(3 * kSecond);
  timer.tick(clock);

  clock.advance(10 * kMillisecond);
  const TickResult after = timer.tick(clock);
  RC_CHECK(after.quality == TickQuality::Good);
  RC_CHECK_NEAR(after.dt, 0.01, 1e-12);
}

RC_TEST("the timer recovers after a backwards step") {
  TestClock clock(10 * kSecond);
  LoopTimer timer(0.1);
  timer.tick(clock);

  clock.set(9 * kSecond);
  timer.tick(clock);

  clock.advance(10 * kMillisecond);
  const TickResult after = timer.tick(clock);
  RC_CHECK(after.quality == TickQuality::Good);
  RC_CHECK_NEAR(after.dt, 0.01, 1e-12);
}

RC_TEST("resetting makes the next tick a first tick again") {
  TestClock clock(0);
  LoopTimer timer(0.1);
  timer.tick(clock);
  clock.advance(10 * kMillisecond);
  timer.tick(clock);

  timer.reset();
  clock.advance(10 * kMillisecond);
  RC_CHECK(timer.tick(clock).quality == TickQuality::First);
}

RC_TEST("a thousand ticks of an hour each stay exact") {
  // The suite covers an hour of behaviour per tick and runs instantly, which is
  // the whole reason the clock is injected rather than read.
  TestClock clock(0);
  LoopTimer timer(4000.0);
  timer.tick(clock);

  for (int i = 0; i < 1000; ++i) {
    clock.advance(3600 * kSecond);
    const TickResult result = timer.tick(clock);
    RC_CHECK(result.quality == TickQuality::Good);
    RC_CHECK_NEAR(result.dt, 3600.0, 1e-9);
  }
}

RC_TEST("the timer works through the interface, so the real clock drops in") {
  const std::unique_ptr<Clock> clock = std::make_unique<SteadyClock>();
  LoopTimer timer(1.0);

  RC_CHECK(timer.tick(*clock).quality == TickQuality::First);

  // The real clock is monotonic, so a second reading is never earlier. This is
  // the one property worth asserting about it, and the reason it is the clock
  // used for intervals.
  const TickResult second = timer.tick(*clock);
  RC_CHECK(second.quality != TickQuality::Backwards || second.dt == 0.0);
}
