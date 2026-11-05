#include <rc/test/rc_test.hpp>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <vector>

#include "solution.hpp"

namespace {

constexpr double kDriftPpm = 50.0;          // a good crystal, which is what the device is assumed to have
constexpr double kMinLatency = 0.002;       // the least a message can take, in seconds
constexpr double kHz = 100.0;
constexpr double kSeconds = 3600.0;
constexpr double kSettled = 600.0;          // the first ten minutes are the map learning

struct Rng {
  std::uint32_t state;
  std::uint32_t next() {
    state ^= state << 13;
    state ^= state >> 17;
    state ^= state << 5;
    return state;
  }
};

struct Errors {
  double host_arrival = 0.0;     // worst error using when the message arrived
  double first_offset = 0.0;     // worst error using the first message's offset for ever
  double fitted = 0.0;           // worst error using the ClockMap
  double drift_ppm = 0.0;
  std::size_t restarts = 0;
};

// A device whose clock runs fast, stamps messages with sixteen bits of
// milliseconds, and sends at 100 Hz over a link that delays each by two to ten
// milliseconds. The "truth" for a message is when it was sent plus the least delay,
// because that is the most any observer without a second clock can know.
Errors run_hour() {
  Rng rng{12345};
  TickUnwrapper<std::uint16_t> unwrapper;
  ClockMap map;
  Errors out;
  double first_offset = 0.0;
  bool have_first = false;

  const long steps = static_cast<long>(kSeconds * kHz);
  for (long i = 0; i < steps; ++i) {
    const double t = static_cast<double>(i) / kHz;                       // when it was sent, by the host's clock
    const double device_ms = t * 1000.0 * (1.0 + kDriftPpm * 1e-6) + 777.0;
    const std::uint16_t raw = static_cast<std::uint16_t>(static_cast<std::uint64_t>(device_ms) & 0xFFFFu);
    const double arrived = t + kMinLatency + static_cast<double>(rng.next() % 8000u) / 1e6;

    const Unwrapped u = unwrapper.accept(raw);
    const double device_seconds = static_cast<double>(u.ticks) / 1000.0;

    if (!have_first) {
      first_offset = arrived - device_seconds;
      have_first = true;
    }
    map.add(device_seconds, arrived);

    if (t > kSettled) {
      const double truth = t + kMinLatency;
      out.host_arrival = std::max(out.host_arrival, std::fabs(arrived - truth));
      out.first_offset = std::max(out.first_offset, std::fabs(device_seconds + first_offset - truth));
      out.fitted = std::max(out.fitted, std::fabs(map.to_host(device_seconds) - truth));
    }
  }
  out.drift_ppm = map.drift_ppm();
  out.restarts = unwrapper.restarts();
  return out;
}

// The same device, which at thirty minutes is off for five seconds and comes back
// with its counter starting again. Whether the map is told is the only difference.
double worst_error_after_restart(bool tell_the_map) {
  Rng rng{12345};
  TickUnwrapper<std::uint16_t> unwrapper;
  ClockMap map;
  bool rebooted = false;
  double worst = 0.0;

  const long steps = static_cast<long>(kSeconds * kHz);
  for (long i = 0; i < steps; ++i) {
    const double t = static_cast<double>(i) / kHz;
    if (t >= 1800.0 && t < 1805.0) continue;   // the device is down, so nothing arrives
    if (t >= 1805.0) rebooted = true;

    const double device_ms = rebooted ? (t - 1805.0) * 1000.0 * (1.0 + kDriftPpm * 1e-6) + 120.0
                                      : t * 1000.0 * (1.0 + kDriftPpm * 1e-6) + 777.0;
    const std::uint16_t raw = static_cast<std::uint16_t>(static_cast<std::uint64_t>(device_ms) & 0xFFFFu);
    const double arrived = t + kMinLatency + static_cast<double>(rng.next() % 8000u) / 1e6;

    const Unwrapped u = unwrapper.accept(raw);
    if (u.restarted && tell_the_map) map.reset();
    const double device_seconds = static_cast<double>(u.ticks) / 1000.0;
    map.add(device_seconds, arrived);

    if (t > 1805.0 && t < 2405.0) {
      worst = std::max(worst, std::fabs(map.to_host(device_seconds) - (t + kMinLatency)));
    }
  }
  return worst;
}

}  // namespace

RC_TEST("a sixteen bit tick counter is followed through its wrap without a jump") {
  TickUnwrapper<std::uint16_t> unwrapper;
  std::uint64_t previous = 0;
  bool always_forward_by_one = true;

  // Starts near the top, so the wrap is in the middle of the run.
  for (int i = 0; i < 200; ++i) {
    const std::uint16_t raw = static_cast<std::uint16_t>(65436 + i);
    const Unwrapped u = unwrapper.accept(raw);
    if (i > 0 && u.ticks != previous + 1) always_forward_by_one = false;
    RC_CHECK(!u.restarted);
    previous = u.ticks;
  }

  RC_CHECK(always_forward_by_one);
  RC_CHECK_EQ(previous, static_cast<std::uint64_t>(199));
  RC_CHECK_EQ(unwrapper.restarts(), static_cast<std::size_t>(0));
}

RC_TEST("a duration across the wrap is ten milliseconds, not minus a minute") {
  // 65531 and then 5: ten ticks apart, on opposite sides of the wrap.
  const std::uint16_t before = 65531;
  const std::uint16_t after = 5;

  const int naive = static_cast<int>(after) - static_cast<int>(before);   // what promotion gives
  TickUnwrapper<std::uint16_t> unwrapper;
  const Unwrapped a = unwrapper.accept(before);
  const Unwrapped b = unwrapper.accept(after);
  const double dt = static_cast<double>(b.ticks - a.ticks) / 1000.0;

  const double moved = 0.01;   // a centimetre
  std::cout << "  a centimetre in ten milliseconds, stamped 65531 then 5:\n"
            << "    subtracting the raw stamps in int  " << naive << " ms, speed " << std::fixed
            << std::setprecision(5) << moved / (naive / 1000.0) << " m/s\n"
            << "    unwrapped                          " << std::setprecision(0) << dt * 1000.0 << " ms, speed "
            << std::setprecision(2) << moved / dt << " m/s\n\n";

  RC_CHECK_NEAR(dt, 0.010, 1e-12);
  RC_CHECK_NEAR(moved / dt, 1.0, 1e-9);
  RC_CHECK_EQ(naive, -65526);
}

RC_TEST("a thirty two bit counter wraps the same way") {
  TickUnwrapper<std::uint32_t> unwrapper;
  std::uint64_t last = 0;
  for (int i = 0; i < 100; ++i) {
    const std::uint32_t raw = static_cast<std::uint32_t>(4294967246u + static_cast<std::uint32_t>(i));
    const Unwrapped u = unwrapper.accept(raw);
    if (i > 0) RC_CHECK_EQ(u.ticks, last + 1);
    last = u.ticks;
  }
  RC_CHECK_EQ(last, static_cast<std::uint64_t>(99));
}

RC_TEST("a device that restarts is noticed, and the timeline carries on from where it was") {
  TickUnwrapper<std::uint16_t> unwrapper;
  unwrapper.accept(1000);
  unwrapper.accept(1010);
  const Unwrapped before = unwrapper.accept(1020);
  RC_CHECK(!before.restarted);

  const Unwrapped reset = unwrapper.accept(30);   // the counter went backwards, and a link is in order
  RC_CHECK(reset.restarted);
  RC_CHECK_EQ(reset.ticks, before.ticks);
  RC_CHECK_EQ(unwrapper.restarts(), static_cast<std::size_t>(1));

  const Unwrapped next = unwrapper.accept(40);
  RC_CHECK(!next.restarted);
  RC_CHECK_EQ(next.ticks, before.ticks + 10);
}

RC_TEST("a counter can see a stall of just under half a turn, and no more") {
  TickUnwrapper<std::uint16_t> seen;
  seen.accept(0);
  const Unwrapped near_half = seen.accept(32767);   // 32.7 seconds of silence
  RC_CHECK(!near_half.restarted);
  RC_CHECK_EQ(near_half.ticks, static_cast<std::uint64_t>(32767));

  TickUnwrapper<std::uint16_t> too_long;
  too_long.accept(0);
  const Unwrapped past_half = too_long.accept(40000);   // looks like a restart
  RC_CHECK(past_half.restarted);
}

RC_TEST("with no jitter the map recovers the offset and the drift exactly") {
  ClockMap map;
  RC_CHECK(!map.ready());
  RC_CHECK_NEAR(map.to_host(12.5), 12.5, 1e-12);   // nothing to go on, so the device time as it is

  // The device runs fast by 80 ppm and started at 7.25 seconds on its own clock.
  for (int i = 0; i < 3000; ++i) {
    const double host = static_cast<double>(i) * 0.1;
    const double device = host * 1.00008 + 7.25;
    map.add(device, host);
  }

  RC_REQUIRE(map.ready());
  RC_CHECK_NEAR(map.drift_ppm(), 80.0, 0.5);
  const double device_now = 299.9 * 1.00008 + 7.25;
  RC_CHECK_NEAR(map.to_host(device_now), 299.9, 1e-4);
}

RC_TEST("one bucket is an offset only, and reset forgets everything") {
  ClockMap map;
  map.add(5.0, 105.0);
  map.add(5.4, 105.4);
  RC_CHECK(!map.ready());
  RC_CHECK_NEAR(map.to_host(6.0), 106.0, 1e-9);
  RC_CHECK_NEAR(map.drift_ppm(), 0.0, 1e-9);

  map.reset();
  RC_CHECK_NEAR(map.to_host(6.0), 6.0, 1e-12);
  RC_CHECK(!map.ready());
}

RC_TEST("what the host's own clock gets wrong, and what the map gets right") {
  const Errors e = run_hour();

  std::cout << "  one hour of a 100 Hz device with a " << kDriftPpm << " ppm fast clock, sixteen bit millisecond\n"
            << "  stamps, and a link that delays each message by 2 to 10 ms. The worst error in the\n"
            << "  send time of a message, after the first ten minutes:\n\n"
            << "    using                                       worst error\n"
            << std::fixed << std::setprecision(1)
            << "    when the message arrived                    " << std::setw(8) << e.host_arrival * 1000.0 << " ms\n"
            << "    the first message's offset, for ever        " << std::setw(8) << e.first_offset * 1000.0 << " ms\n"
            << std::setprecision(2)
            << "    the fitted map                              " << std::setw(8) << e.fitted * 1000.0 << " ms\n\n"
            << "  The map found a drift of " << std::setprecision(1) << e.drift_ppm
            << " ppm, and the device was " << kDriftPpm << " ppm fast.\n"
            << "  Its error is measured from the send time plus the least delay, " << kMinLatency * 1000.0
            << " ms, because nothing on\n  one side of a link can know the least delay without a second clock.\n\n";

  // The arrival time is wrong by the jitter, and the offset taken once is wrong by
  // the drift, which grows for as long as the program runs.
  RC_CHECK(e.host_arrival > 0.007);
  RC_CHECK(e.first_offset > 0.15);

  // The map is wrong by about a millisecond and does not grow.
  RC_CHECK(e.fitted < 0.002);
  RC_CHECK(e.fitted * 5.0 < e.host_arrival);
  RC_CHECK_NEAR(e.drift_ppm, kDriftPpm, 10.0);
  RC_CHECK_EQ(e.restarts, static_cast<std::size_t>(0));
}

RC_TEST("a restart the map is not told about is wrong by as long as the device was off") {
  const double ignored = worst_error_after_restart(false);
  const double told = worst_error_after_restart(true);

  std::cout << "  the device is off for five seconds at thirty minutes and restarts its counter.\n"
            << "  Worst error in the send time over the next ten minutes:\n\n"
            << "    map kept, restart not noticed   " << std::fixed << std::setprecision(1) << std::setw(8)
            << ignored * 1000.0 << " ms\n"
            << "    map reset on the restart        " << std::setw(8) << told * 1000.0 << " ms\n\n";

  // The unwrapped timeline carries on across the restart and the five seconds of
  // silence are not in it, so every old bucket is five seconds out and the fit
  // averages them in for as long as they stay in the window.
  RC_CHECK(ignored > 4.5);
  RC_CHECK(told < 0.010);
}
