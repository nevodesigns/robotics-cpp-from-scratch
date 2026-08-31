#include <rc/test/rc_test.hpp>
#include <rc/test/leak_check.hpp>

#include <chrono>
#include <iostream>
#include <thread>

#include "solution.hpp"

namespace {

using rc::test::LeakCheck;

// File scope, not inside the test. A constexpr local used in a lambda is
// rejected by MSVC as C3493. See E-CPP-0023.
constexpr double kJitterLoopPeriodUs = 1000.0;

// Ten microsecond buckets across one millisecond, which is the shape a
// kilohertz loop wants.
Histogram make_histogram() { return Histogram(10.0, 100); }

}  // namespace

RC_TEST("a new histogram has nothing in it") {
  const Histogram h = make_histogram();
  RC_CHECK_EQ(h.count(), 0L);
  RC_CHECK_EQ(h.overflows(), 0L);
  RC_CHECK_NEAR(h.worst(), 0.0, 1e-12);
  RC_CHECK_NEAR(h.percentile(50.0), 0.0, 1e-12);
}

RC_TEST("samples are counted") {
  Histogram h = make_histogram();
  for (int i = 0; i < 7; ++i) h.record(15.0);
  RC_CHECK_EQ(h.count(), 7L);
}

RC_TEST("the worst value is remembered exactly, not rounded to a bucket") {
  Histogram h = make_histogram();
  h.record(12.0);
  h.record(347.5);
  h.record(9.0);
  RC_CHECK_NEAR(h.worst(), 347.5, 1e-12);
}

RC_TEST("values beyond the range are counted as overflows") {
  // The check that catches a histogram folding its tail into the last bucket.
  Histogram h(10.0, 10);   // covers 0 to 100
  h.record(50.0);
  h.record(4000.0);

  RC_CHECK_EQ(h.overflows(), 1L);
  RC_CHECK_EQ(h.count(), 2L);
  RC_CHECK_NEAR(h.worst(), 4000.0, 1e-12);
}

RC_TEST("a stall does not read as the top of the range") {
  Histogram h(10.0, 10);
  for (int i = 0; i < 99; ++i) h.record(15.0);
  h.record(4000.0);

  // If the overflow had been folded into the last bucket, the worst would read
  // as 100, understating the stall by a factor of forty.
  RC_CHECK_NEAR(h.worst(), 4000.0, 1e-12);
  RC_CHECK(h.worst() > 100.0);
}

RC_TEST("percentiles land in the right bucket") {
  Histogram h(10.0, 100);
  // Ninety samples at 5, ten at 205.
  for (int i = 0; i < 90; ++i) h.record(5.0);
  for (int i = 0; i < 10; ++i) h.record(205.0);

  RC_CHECK_NEAR(h.percentile(50.0), 10.0, 1e-12);    // the first bucket
  RC_CHECK_NEAR(h.percentile(90.0), 10.0, 1e-12);
  RC_CHECK_NEAR(h.percentile(95.0), 210.0, 1e-12);   // out in the tail
}

RC_TEST("the percentile index is not truncated") {
  // Seven samples, so the median is the fourth smallest. Three sit in the
  // first bucket and four out at 105, which puts the answer in the far bucket.
  //
  // The check that catches integer index arithmetic. Fifty percent of seven is
  // 3.5; truncated to 3 it matches after the third sample and answers with the
  // first bucket, which is a value the median is nowhere near.
  Histogram h(10.0, 100);
  for (int i = 0; i < 3; ++i) h.record(5.0);
  for (int i = 0; i < 4; ++i) h.record(105.0);

  RC_CHECK_NEAR(h.percentile(50.0), 110.0, 1e-12);
}

RC_TEST("the median is unmoved by a long tail, which is why it is reported") {
  Histogram h(10.0, 100);
  for (int i = 0; i < 999; ++i) h.record(5.0);
  h.record(900.0);

  RC_CHECK_NEAR(h.percentile(50.0), 10.0, 1e-12);
  RC_CHECK_NEAR(h.worst(), 900.0, 1e-12);
}

RC_TEST("percentile arguments outside the range are clamped rather than indexing wildly") {
  Histogram h = make_histogram();
  h.record(15.0);
  RC_CHECK_NEAR(h.percentile(-10.0), 20.0, 1e-12);
  RC_CHECK_NEAR(h.percentile(150.0), 20.0, 1e-12);
}

RC_TEST("samples over a budget are counted, overflows included") {
  Histogram h(10.0, 10);   // covers 0 to 100
  for (int i = 0; i < 5; ++i) h.record(15.0);    // under a budget of 50
  for (int i = 0; i < 3; ++i) h.record(75.0);    // over it
  h.record(5000.0);                              // over it, and an overflow

  RC_CHECK_EQ(h.over(50.0), 4L);
}

RC_TEST("recording allocates nothing") {
  Histogram h = make_histogram();
  const LeakCheck check;
  for (int i = 0; i < 200000; ++i) h.record(static_cast<double>(i % 500));
  RC_CHECK(check.balanced());
}

RC_TEST("the first tick has nothing to compare against") {
  LoopMonitor monitor(1000.0);
  const Tick first = monitor.tick(50000.0);
  RC_CHECK(first.first);
  RC_CHECK_NEAR(first.interval, 0.0, 1e-12);
  RC_CHECK_NEAR(first.lateness, 0.0, 1e-12);
}

RC_TEST("a loop running exactly on time has no lateness") {
  LoopMonitor monitor(1000.0);
  monitor.tick(0.0);
  const Tick second = monitor.tick(1000.0);
  RC_CHECK(!second.first);
  RC_CHECK_NEAR(second.interval, 1000.0, 1e-12);
  RC_CHECK_NEAR(second.lateness, 0.0, 1e-12);
}

RC_TEST("lateness is the interval minus the period") {
  LoopMonitor monitor(1000.0);
  monitor.tick(0.0);
  RC_CHECK_NEAR(monitor.tick(1043.0).lateness, 43.0, 1e-12);
}

RC_TEST("a loop that runs early reports negative lateness") {
  LoopMonitor monitor(1000.0);
  monitor.tick(0.0);
  RC_CHECK(monitor.tick(980.0).lateness < 0.0);
}

RC_TEST("intervals are measured tick to tick, not from the start") {
  LoopMonitor monitor(1000.0);
  monitor.tick(0.0);
  monitor.tick(1000.0);
  RC_CHECK_NEAR(monitor.tick(2100.0).interval, 1100.0, 1e-12);
}

RC_TEST("a real loop, measured on this machine") {
  // Not an assertion about the machine, which would be flaky and would tell a
  // learner nothing. This runs a genuine loop and prints what it found, so the
  // lesson ends looking at real timing rather than at a claim about it.
  Histogram jitter(10.0, 100);
  LoopMonitor monitor(kJitterLoopPeriodUs);

  const auto started = std::chrono::steady_clock::now();
  auto next = started;

  for (int cycle = 0; cycle < 300; ++cycle) {
    next += std::chrono::microseconds(static_cast<long>(kJitterLoopPeriodUs));
    std::this_thread::sleep_until(next);

    const double now_us = std::chrono::duration<double, std::micro>(
                              std::chrono::steady_clock::now() - started)
                              .count();
    const Tick t = monitor.tick(now_us);
    if (!t.first) jitter.record(t.lateness);
  }

  std::cout << "\n  lateness of a 1 kHz loop, 300 cycles, microseconds\n"
            << jitter.render(40)
            << "  median " << jitter.percentile(50.0)
            << "   99th " << jitter.percentile(99.0)
            << "   worst " << jitter.worst()
            << "   over 500us: " << jitter.over(500.0) << "\n";

  // The only assertion is that the measurement happened at all. What the
  // numbers are depends on the machine, and on an ordinary kernel they depend
  // on what else it is doing.
  RC_CHECK_EQ(jitter.count(), 299L);
}
