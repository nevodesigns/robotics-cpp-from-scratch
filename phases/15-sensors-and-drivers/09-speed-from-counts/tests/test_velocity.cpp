#include <rc/test/rc_test.hpp>

#include <cmath>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <vector>

#include "solution.hpp"

namespace {

const double kSample = 0.001;       // the loop runs at 1 kHz
const double kCountsPerRev = 1000.0;
const double kTimerTick = 1e-5;     // the timer that stamps edges resolves ten microseconds
const double kSlotError = 0.02;     // an encoder's edges are not evenly spread: two percent of a slot
const double kThreshold = 1500.0;   // counts a second below which the edge timing is believed

struct Rng {
  std::uint32_t state;
  std::uint32_t next() {
    state ^= state << 13;
    state ^= state >> 17;
    state ^= state << 5;
    return state;
  }
  double uniform() { return static_cast<double>(next() >> 8) / 16777216.0; }
};

struct Errors {
  double diff1 = 0.0;      // percent rms error
  double window = 0.0;
  double period = 0.0;
  double hybrid = 0.0;
};

// An encoder turning at a steady speed, observed the way a controller does: a count at each
// sample, and a timestamp at each edge. The edges are a little uneven, and the timestamps
// are rounded to the timer's tick.
Errors measure(double revolutions_a_second) {
  Rng rng{9};
  const double counts_a_second = revolutions_a_second * kCountsPerRev;
  const long edges = static_cast<long>(counts_a_second * 21.0) + 10;
  std::vector<double> edge_time(static_cast<std::size_t>(edges));
  for (long n = 0; n < edges; ++n) {
    edge_time[static_cast<std::size_t>(n)] =
        (static_cast<double>(n + 1) + kSlotError * (2.0 * rng.uniform() - 1.0)) / counts_a_second;
  }

  WindowVelocity one(1, kSample);
  WindowVelocity fifty(50, kSample);
  PeriodVelocity period;
  long index = 0;
  double s1 = 0.0, sw = 0.0, sp = 0.0, sh = 0.0;
  int n = 0;

  for (int k = 0; k < 20000; ++k) {
    const double t = static_cast<double>(k + 1) * kSample;
    while (index < edges && edge_time[static_cast<std::size_t>(index)] <= t) {
      period.on_edge(std::floor(edge_time[static_cast<std::size_t>(index)] / kTimerTick) * kTimerTick, +1);
      ++index;
    }

    const double d1 = one.update(index);
    const double dw = fifty.update(index);
    const double dp = period.estimate(t);
    const double dh = pick_velocity(dp, dw, kThreshold);

    if (k < 60) continue;   // let the window fill
    s1 += (d1 - counts_a_second) * (d1 - counts_a_second);
    sw += (dw - counts_a_second) * (dw - counts_a_second);
    sp += (dp - counts_a_second) * (dp - counts_a_second);
    sh += (dh - counts_a_second) * (dh - counts_a_second);
    ++n;
  }

  const auto percent = [&](double sum) { return 100.0 * std::sqrt(sum / static_cast<double>(n)) / counts_a_second; };
  Errors e;
  e.diff1 = percent(s1);
  e.window = percent(sw);
  e.period = percent(sp);
  e.hybrid = percent(sh);
  return e;
}

}  // namespace

RC_TEST("a window of counts gives the speed over the window, and less until it has filled") {
  WindowVelocity v(10, kSample);

  RC_CHECK_NEAR(v.update(0), 0.0, 1e-12);          // one count tells nothing about speed
  double speed = 0.0;
  for (int i = 1; i <= 40; ++i) {
    speed = v.update(5L * i);                       // five counts a sample, which is 5000 a second
    RC_CHECK_NEAR(speed, 5000.0, 1e-9);            // from the second sample on, with what it has
  }
  RC_CHECK_NEAR(v.delay_seconds(), 0.005, 1e-12);

  // A window of one is a plain difference.
  WindowVelocity plain(1, kSample);
  plain.update(100);
  RC_CHECK_NEAR(plain.update(103), 3000.0, 1e-9);

  // A window of nothing is a window of one.
  WindowVelocity none(0, kSample);
  none.update(0);
  RC_CHECK_NEAR(none.update(2), 2000.0, 1e-9);
}

RC_TEST("a window pays for its resolution in delay: a change of speed arrives half a window late") {
  WindowVelocity v(50, kSample);
  long count = 0;
  int crossed_at = -1;

  // 1000 counts a second for 200 samples, then 2000 for the rest. The window reports the
  // average over itself, so it passes the halfway value, 1500, when half of it is the new
  // speed.
  for (int k = 0; k < 400; ++k) {
    count += k < 200 ? 1 : 2;
    const double speed = v.update(count);
    if (crossed_at < 0 && k >= 200 && speed >= 1500.0) crossed_at = k - 200;
  }

  std::cout << "  the speed doubles at sample 200. A window of 50 reports the halfway value " << crossed_at
            << " samples later, which is half a window, " << v.delay_seconds() * 1000.0 << " ms.\n\n";

  RC_CHECK(crossed_at >= 24 && crossed_at <= 26);
}

RC_TEST("the time between two counts is the speed, with a sign, and a reversal is not a speed") {
  PeriodVelocity p;
  RC_CHECK_NEAR(p.estimate(0.0), 0.0, 1e-12);          // nothing yet

  p.on_edge(1.000, +1);
  RC_CHECK_NEAR(p.estimate(1.0005), 0.0, 1e-12);       // one edge is a position, not a speed

  p.on_edge(1.002, +1);
  RC_CHECK_NEAR(p.estimate(1.0021), 500.0, 1e-9);      // a count every two milliseconds

  p.on_edge(1.003, +1);
  RC_CHECK_NEAR(p.estimate(1.0031), 1000.0, 1e-9);     // and now one every millisecond

  // It turns round. The next edge is the same position crossed the other way, and the
  // time since the last one says nothing about how fast it is going back.
  p.on_edge(1.0035, -1);
  RC_CHECK_NEAR(p.estimate(1.0036), 0.0, 1e-12);

  p.on_edge(1.0075, -1);
  RC_CHECK_NEAR(p.estimate(1.0076), -250.0, 1e-9);     // now it has two, and it is going the other way
}

RC_TEST("a stopped encoder reads slower and slower, and not its last speed for ever") {
  PeriodVelocity p;
  p.on_edge(1.000, +1);
  p.on_edge(1.002, +1);
  RC_CHECK_NEAR(p.estimate(1.002), 500.0, 1e-9);

  // No more edges. Within a gap it is still 500. After that it can only be slower, and it
  // is at most one count over the time that has passed.
  RC_CHECK_NEAR(p.estimate(1.0019 + 0.0001), 500.0, 1e-9);
  RC_CHECK_NEAR(p.estimate(1.102), 10.0, 1e-9);        // a tenth of a second on
  RC_CHECK_NEAR(p.estimate(2.002), 1.0, 1e-9);         // a second on

  double previous = 1e9;
  for (int i = 0; i < 100; ++i) {
    const double v = p.estimate(1.003 + 0.01 * static_cast<double>(i));
    RC_CHECK(v <= previous);
    previous = v;
  }
  RC_CHECK(previous < 2.0);
}

RC_TEST("the edge timing is believed when it is slow and the window when it is fast") {
  RC_CHECK_NEAR(pick_velocity(500.0, 520.0, 1500.0), 500.0, 1e-12);
  RC_CHECK_NEAR(pick_velocity(-500.0, -520.0, 1500.0), -500.0, 1e-12);   // either way round
  RC_CHECK_NEAR(pick_velocity(3000.0, 3010.0, 1500.0), 3010.0, 1e-12);
  RC_CHECK_NEAR(pick_velocity(0.0, 7.0, 1500.0), 7.0, 1e-12);            // no edges to go by: the window
  RC_CHECK_NEAR(pick_velocity(1499.0, 1400.0, 1500.0), 1499.0, 1e-12);
  RC_CHECK_NEAR(pick_velocity(1501.0, 1400.0, 1500.0), 1400.0, 1e-12);
}

RC_TEST("no single way of getting speed from counts is good at every speed") {
  const double speeds[] = {0.05, 0.5, 5.0, 50.0};
  Errors e[4];
  for (int i = 0; i < 4; ++i) e[i] = measure(speeds[i]);

  std::cout << "  an encoder of 1000 counts a revolution read at 1 kHz, a 10 microsecond timer, edges 2 percent\n"
            << "  uneven. The rms error in the speed, as a percent of the speed, at four speeds:\n\n"
            << "    speed             counts/s   one sample   window of 50    edge timing    the two combined\n";
  for (int i = 0; i < 4; ++i) {
    std::cout << "    " << std::fixed << std::setprecision(2) << std::setw(6) << speeds[i] << " rev/s" << std::setprecision(0)
              << std::setw(11) << speeds[i] * kCountsPerRev << std::setprecision(1) << std::setw(11) << e[i].diff1
              << " %" << std::setw(11) << e[i].window << " %" << std::setw(11) << e[i].period << " %" << std::setw(14)
              << e[i].hybrid << " %\n";
  }
  std::cout << "\n  A single sample is useless below a few counts a sample. The window is right at speed and delays by\n"
            << "  half of itself. Edge timing is right when slow and fails when the gap between counts is a few timer\n"
            << "  ticks. Taking each where it is good gives the last column.\n\n";

  // One sample: whole counts only. At half a count a sample it is zero or one, which is
  // a hundred percent out, and at a twentieth of that four times worse.
  RC_CHECK(e[0].diff1 > 100.0);
  RC_CHECK(e[1].diff1 > 90.0);
  RC_CHECK(e[2].diff1 > 5.0);

  // The window is good at speed and at the bottom still a fifth out.
  RC_CHECK(e[0].window > 10.0);
  RC_CHECK(e[1].window < 3.0);
  RC_CHECK(e[3].window < 0.1);

  // Edge timing is good slowly and bad fast.
  RC_CHECK(e[0].period < 3.0);
  RC_CHECK(e[1].period < 3.0);
  RC_CHECK(e[3].period > 40.0);

  // Each where it is good: within two percent at every speed.
  for (int i = 0; i < 4; ++i) RC_CHECK(e[i].hybrid < 2.0);
}
