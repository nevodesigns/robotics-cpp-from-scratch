#include <rc/test/rc_test.hpp>

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <vector>

#include "solution.hpp"

namespace {

using Ns = rc::core::Nanoseconds;

const Ns kMs = 1000000;

std::vector<Ns> repeat(int count, Ns work) { return std::vector<Ns>(static_cast<std::size_t>(count), work); }

// The loop everybody writes first: do the work, then sleep a whole period. The
// time is simulated, so a hundred cycles take no time at all and the answer is
// exact.
std::vector<Ns> run_naive(Ns period, const std::vector<Ns>& works) {
  std::vector<Ns> starts;
  Ns now = 0;
  for (const Ns work : works) {
    starts.push_back(now);
    now += work;
    now += period;
  }
  return starts;
}

// A fixed grid and nothing else: the next start is one period after the last
// scheduled one, and if that is already past there is no sleep, so a late cycle is
// followed by others run back to back until the grid is caught up.
std::vector<Ns> run_fixed_grid(Ns period, const std::vector<Ns>& works) {
  std::vector<Ns> starts;
  Ns now = 0;
  Ns scheduled = 0;
  for (const Ns work : works) {
    starts.push_back(now);
    now += work;
    scheduled += period;
    if (scheduled > now) now = scheduled;
  }
  return starts;
}

// The same loop, paced by the learner's class.
std::vector<Ns> run_paced(Ns period, const std::vector<Ns>& works, long long* missed = nullptr) {
  std::vector<Ns> starts;
  Pacer pacer(period, 0);
  Ns now = 0;
  for (const Ns work : works) {
    starts.push_back(now);
    now += work;
    now += pacer.sleep_needed(now);
  }
  if (missed != nullptr) *missed = pacer.missed();
  return starts;
}

Ns shortest_interval(const std::vector<Ns>& starts) {
  Ns shortest = starts.back();
  for (std::size_t i = 1; i < starts.size(); ++i) shortest = std::min(shortest, starts[i] - starts[i - 1]);
  return shortest;
}

int intervals_shorter_than(const std::vector<Ns>& starts, Ns period) {
  int count = 0;
  for (std::size_t i = 1; i < starts.size(); ++i)
    if (starts[i] - starts[i - 1] < period) ++count;
  return count;
}

}  // namespace

RC_TEST("sleeping a period after the work makes every cycle the work plus the period") {
  const std::vector<Ns> works = repeat(100, 2 * kMs);

  const std::vector<Ns> naive = run_naive(10 * kMs, works);
  const std::vector<Ns> paced = run_paced(10 * kMs, works);
  RC_REQUIRE_EQ(naive.size(), 100u);
  RC_REQUIRE_EQ(paced.size(), 100u);

  const double naive_hz = 1000.0 / (static_cast<double>(naive[1] - naive[0]) / kMs);
  const double paced_hz = 1000.0 / (static_cast<double>(paced[1] - paced[0]) / kMs);

  std::cout << "  100 cycles, 2 ms of work, a 10 ms period, wanting 100 Hz, simulated:\n\n";
  std::cout << "                              100th cycle started at    each interval    rate\n";
  std::cout << "    sleep a period after      " << std::setw(15) << naive[99] / kMs << " ms" << std::setw(12)
            << (naive[1] - naive[0]) / kMs << " ms" << std::setw(11) << std::fixed << std::setprecision(1)
            << naive_hz << " Hz\n";
  std::cout << "    your Pacer                " << std::setw(15) << paced[99] / kMs << " ms" << std::setw(12)
            << (paced[1] - paced[0]) / kMs << " ms" << std::setw(11) << paced_hz << " Hz\n\n";
  std::cout << "  On real threads and a real clock, measured outside the suite on gcc 11:\n";
  std::cout << "    sleep_for(period) after the work     1210 ms, 82.6 Hz\n";
  std::cout << "    sleep_for(period - work)             about 105 to 110 us late per cycle\n";
  std::cout << "    sleep_until a fixed grid             1000 ms, 99.99 Hz\n\n";

  RC_CHECK_EQ(naive[99], 99 * 12 * kMs);
  RC_CHECK_EQ(paced[99], 99 * 10 * kMs);
  for (std::size_t i = 1; i < paced.size(); ++i) RC_CHECK_EQ(paced[i] - paced[i - 1], 10 * kMs);
}

RC_TEST("the sleep is what is left until the next point on the grid") {
  Pacer pacer(10 * kMs, 0);

  // Work of 2 ms: sleep the other 8.
  RC_CHECK_EQ(pacer.sleep_needed(2 * kMs), 8 * kMs);
  RC_CHECK_EQ(pacer.deadline(), 10 * kMs);

  // The cycle began at 10 and no work at all took no time: sleep the whole 10.
  RC_CHECK_EQ(pacer.sleep_needed(10 * kMs), 10 * kMs);
  RC_CHECK_EQ(pacer.deadline(), 20 * kMs);

  // Work that takes exactly the period is on time, with nothing to sleep and
  // nothing lost.
  RC_CHECK_EQ(pacer.sleep_needed(30 * kMs), 0);
  RC_CHECK_EQ(pacer.deadline(), 30 * kMs);
  RC_CHECK_EQ(pacer.missed(), 0);

  // One nanosecond late loses the whole slot. That is the cost of dropping and not
  // bursting, and it is written down so it is a decision.
  RC_CHECK_EQ(pacer.sleep_needed(40 * kMs + 1), 10 * kMs - 1);
  RC_CHECK_EQ(pacer.missed(), 1);
  RC_CHECK_EQ(pacer.deadline(), 50 * kMs);
}

RC_TEST("an overrun drops the slots it overran, counts them, and lands back on the grid") {
  const std::vector<Ns> works = {8 * kMs, 14 * kMs, 35 * kMs, 2 * kMs, 2 * kMs};

  long long missed = 0;
  const std::vector<Ns> paced = run_paced(10 * kMs, works, &missed);
  const std::vector<Ns> naive = run_naive(10 * kMs, works);
  RC_REQUIRE_EQ(paced.size(), 5u);

  std::cout << "  work of 8, 14, 35, 2, 2 ms in a 10 ms period, when each cycle started (ms):\n\n";
  std::cout << "    sleep a period after   ";
  for (const Ns start : naive) std::cout << std::setw(5) << start / kMs;
  std::cout << "\n    your Pacer             ";
  for (const Ns start : paced) std::cout << std::setw(5) << start / kMs;
  std::cout << "\n\n    slots dropped: " << missed << "\n\n";

  RC_CHECK_EQ(paced[0], 0);
  RC_CHECK_EQ(paced[1], 10 * kMs);
  RC_CHECK_EQ(paced[2], 30 * kMs);
  RC_CHECK_EQ(paced[3], 70 * kMs);
  RC_CHECK_EQ(paced[4], 80 * kMs);
  RC_CHECK_EQ(missed, 4);

  // Every start is on the grid, however long the overrun was.
  for (const Ns start : paced) RC_CHECK_EQ(start % (10 * kMs), 0);

  // And the loop that sleeps after the work is nowhere near it.
  RC_CHECK(naive[1] % (10 * kMs) != 0);
}

RC_TEST("a plain fixed grid catches up after a stall with a burst, and the pacer does not") {
  std::vector<Ns> works = repeat(20, 2 * kMs);
  works[10] = 45 * kMs;

  long long missed = 0;
  const std::vector<Ns> grid = run_fixed_grid(10 * kMs, works);
  const std::vector<Ns> paced = run_paced(10 * kMs, works, &missed);

  const int grid_short = intervals_shorter_than(grid, 10 * kMs);
  const int paced_short = intervals_shorter_than(paced, 10 * kMs);

  std::cout << "  one 45 ms cycle in a 10 ms loop, 20 cycles:\n\n";
  std::cout << "                          shortest interval    intervals shorter than the period\n";
  std::cout << "    a plain fixed grid    " << std::setw(12) << shortest_interval(grid) / kMs << " ms"
            << std::setw(24) << grid_short << "\n";
  std::cout << "    your Pacer            " << std::setw(12) << shortest_interval(paced) / kMs << " ms"
            << std::setw(24) << paced_short << "\n\n";
  std::cout << "  Measured on real threads outside the suite, a fixed deadline after a 45 ms\n";
  std::cout << "  stall started 5 cycles late, one after another with no sleep between them.\n\n";

  // The grid keeps its total and loses its spacing: several cycles in a few
  // milliseconds, each with a dt far below the period.
  RC_CHECK(grid_short >= 4);
  RC_CHECK_EQ(shortest_interval(grid), 2 * kMs);

  // The pacer never starts a cycle sooner than a period after the last.
  RC_CHECK_EQ(paced_short, 0);
  RC_CHECK(shortest_interval(paced) >= 10 * kMs);
  RC_CHECK(missed >= 1);
}

RC_TEST("a clock that went backwards waits one period, not an hour, and a zero period does not divide") {
  // The grid says the next start is a full second away; now says it is zero. The
  // clock was stepped back, or this was called early. Waiting a second is the
  // loop stopping.
  Pacer pacer(10 * kMs, 1000 * kMs);
  RC_CHECK_EQ(pacer.sleep_needed(0), 10 * kMs);
  RC_CHECK_EQ(pacer.deadline(), 10 * kMs);

  // A period of zero or less would be a division by zero on the first late cycle.
  Pacer zero(0, 0);
  RC_CHECK_EQ(zero.period(), 1);
  RC_CHECK(zero.sleep_needed(100) >= 0);

  Pacer negative(-5, 0);
  RC_CHECK_EQ(negative.period(), 1);
  RC_CHECK(negative.sleep_needed(0) >= 0);
}
