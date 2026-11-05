#include <rc/test/rc_test.hpp>

#include <chrono>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <thread>
#include <vector>

#include "solution.hpp"

namespace {

using std::chrono::milliseconds;
using Clock = std::chrono::steady_clock;

const char* name_of(WaitStyle style) {
  switch (style) {
    case WaitStyle::kSpin: return "spins";
    case WaitStyle::kPoll: return "polls every 1 ms";
    default: return "sleeps on a queue";
  }
}

RigOptions options_for(WaitStyle style) {
  RigOptions options;
  options.style = style;
  options.rate_hz = 200;
  options.duration = milliseconds(300);
  return options;
}

// Cycles in a 300 ms run at 200 Hz.
const long kExpectedSlots = 60;

}  // namespace

RC_TEST("every reading is delivered or dropped, and every slot is a reading or a missed slot") {
  const WaitStyle styles[] = {WaitStyle::kSpin, WaitStyle::kPoll, WaitStyle::kSleep};

  for (const WaitStyle style : styles) {
    const Report report = run_rig(options_for(style));

    RC_CHECK(report.delivered > 0);
    RC_CHECK_EQ(report.produced, report.delivered + report.dropped);
    RC_CHECK_EQ(report.dropped, 0L);

    // Slots: the producer is paced on a grid, so produced plus dropped slots is the run's length.
    const long slots = report.produced + report.missed_slots;
    RC_CHECK(std::labs(slots - kExpectedSlots) <= 3);
  }
}

RC_TEST("three ways to wait, measured: what each costs in latency and in wakeups") {
  struct Row {
    WaitStyle style;
    Report report;
  };
  std::vector<Row> rows;
  for (const WaitStyle style : {WaitStyle::kSpin, WaitStyle::kPoll, WaitStyle::kSleep})
    rows.push_back({style, run_rig(options_for(style))});

  std::cout << "  200 Hz for 300 ms, one producer and one consumer, latency from the stamp to taken:\n\n";
  std::cout << "    consumer              delivered   p50 us   p99 us   max us   wakeups\n";
  for (const Row& row : rows) {
    std::cout << "    " << std::left << std::setw(20) << name_of(row.style) << std::right << std::setw(10)
              << row.report.delivered << std::setw(9) << std::fixed << std::setprecision(1) << row.report.p50_us
              << std::setw(9) << row.report.p99_us << std::setw(10) << row.report.max_us << std::setw(10)
              << row.report.wakeups << "\n";
  }
  std::cout << "\n  Measured once over 2 seconds (400 readings), including the CPU the consumer used:\n";
  std::cout << "    spins     p50 0.4 us    p99 1.3 to 1.6 us     CPU 1999 ms of 2000, a whole core\n";
  std::cout << "    polls     p50 514 us    p99 1080 to 1146 us   CPU 31 to 32 ms\n";
  std::cout << "    sleeps    p50 39 us     p99 167 to 245 us     CPU 4.7 to 8.9 ms\n\n";

  for (const Row& row : rows) {
    RC_CHECK(row.report.delivered > 0);
    RC_CHECK(row.report.p50_us <= row.report.p99_us);
    // The percentile is the top edge of a 1 us bucket, so the true worst may sit just under it.
    RC_CHECK(row.report.max_us + 1.0 >= row.report.p99_us);
    RC_CHECK(row.report.max_us < 1000000.0);
    RC_CHECK(row.report.p99_us > 0.0);
  }
}

RC_TEST("polling is bounded by its interval and sleeping by its readings, whatever the machine is doing") {
  const Report spin = run_rig(options_for(WaitStyle::kSpin));
  const Report poll = run_rig(options_for(WaitStyle::kPoll));
  const Report sleep = run_rig(options_for(WaitStyle::kSleep));

  std::cout << "  consumer loop rounds in a 300 ms run:  spins " << spin.wakeups << ", polls " << poll.wakeups
            << ", sleeps " << sleep.wakeups << "  (" << sleep.delivered << " readings)\n";
  std::cout << "  On a quiet machine a spinning consumer went round hundreds of thousands of times. That\n";
  std::cout << "  is printed and not asserted: with every core busy the spinning thread is descheduled\n";
  std::cout << "  too and goes round far less, and a count that depends on the CPU the scheduler grants\n";
  std::cout << "  is not a test. What follows holds under any load, because only a slower machine\n";
  std::cout << "  lowers it.\n\n";

  RC_REQUIRE(spin.delivered > 0);
  RC_REQUIRE(poll.delivered > 0);
  RC_REQUIRE(sleep.delivered > 0);

  // Every consumer goes round at least once per reading it takes.
  RC_CHECK(spin.wakeups >= spin.delivered);
  RC_CHECK(poll.wakeups >= poll.delivered);

  // A poller sleeps at least a millisecond between empty rounds, so in 300 ms it cannot go round more
  // than about 300 times, plus once per reading. A busy machine can only lower this.
  RC_CHECK(poll.wakeups <= 400);

  // A sleeper goes round once per reading, plus a timeout every 50 ms and the close.
  RC_CHECK(sleep.wakeups <= sleep.delivered + 10);
}

RC_TEST("a consumer too slow for the producer loses readings, counts them, and never slows the producer") {
  RigOptions options = options_for(WaitStyle::kSpin);
  options.queue_capacity = 4;
  options.consumer_work_us = 20000;  // 20 ms of work per reading against a 5 ms producer

  const Report report = run_rig(options);

  std::cout << "  a 5 ms producer against a consumer taking 20 ms a reading, queue of 4:\n";
  std::cout << "    produced " << report.produced << ", delivered " << report.delivered << ", dropped "
            << report.dropped << ", slots missed by the producer " << report.missed_slots << "\n\n";

  RC_CHECK(report.dropped > 10);
  RC_CHECK(report.delivered >= 5);
  RC_CHECK_EQ(report.produced, report.delivered + report.dropped);

  // The producer kept its rate: a slow consumer is the consumer's problem and not the producer's.
  RC_CHECK(std::labs((report.produced + report.missed_slots) - kExpectedSlots) <= 3);
}

RC_TEST("a stop request ends the run promptly, with the books still balanced") {
  RigOptions options = options_for(WaitStyle::kSleep);
  options.duration = milliseconds(5000);

  rc::rt::StopSource source;
  std::thread asker([&source] {
    std::this_thread::sleep_for(milliseconds(80));
    source.request_stop();
  });

  const auto began = Clock::now();
  const Report report = run_rig(options, source.get_token());
  const auto took = std::chrono::duration_cast<milliseconds>(Clock::now() - began);
  asker.join();

  std::cout << "  a 5000 ms run, stop requested after 80 ms: returned after " << took.count() << " ms with "
            << report.produced << " produced, " << report.delivered << " delivered\n\n";

  RC_CHECK(took.count() < 1500);
  RC_CHECK(report.produced >= 5);
  RC_CHECK(report.produced < 100);
  RC_CHECK_EQ(report.produced, report.delivered + report.dropped);
}
