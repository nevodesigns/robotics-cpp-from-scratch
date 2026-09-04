#include <rc/test/rc_test.hpp>

#include <rc/core/calibration.hpp>
#include <rc/core/clock.hpp>
#include <rc/core/filters.hpp>
#include <rc/sensor/stamped.hpp>

#include <cmath>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <limits>
#include <vector>

#include "solution.hpp"

namespace {

using rc::core::Calibration;
using rc::core::MovingAverage;
using rc::core::Nanoseconds;

constexpr Nanoseconds kMillisecond = 1000000;
constexpr Nanoseconds kSampleInterval = 2 * kMillisecond;  // 500 Hz
constexpr double kSeconds = 0.002;

// The rangefinder from 15-02: 1.5 percent short, 42 cm long, and jittering.
constexpr double kSensorScale = 0.985;
constexpr double kSensorOffset = 0.42;

class Noise {
 public:
  explicit Noise(std::uint64_t seed) : state_(seed * 6364136223846793005ULL + 1ULL) {}
  double uniform() {
    state_ = state_ * 6364136223846793005ULL + 1442695040888963407ULL;
    return static_cast<double>((state_ >> 11) & 0x1FFFFFFFFFFFFFULL) /
           static_cast<double>(0x20000000000000ULL);
  }
  double gaussian() {
    double total = 0.0;
    for (int i = 0; i < 12; ++i) total += uniform();
    return total - 6.0;
  }

 private:
  std::uint64_t state_;
};

Calibration correction() {
  Calibration fit;
  fit.scale = 1.0 / kSensorScale;
  fit.offset = -kSensorOffset / kSensorScale;
  return fit;
}

double raw_for(double truth) { return kSensorScale * truth + kSensorOffset; }

Channel plain_channel(int window) {
  return Channel(correction(), window, 0.050, 0.0, 30.0, 0);
}

// The longest run of identical readings a parked sensor produces on its own,
// which is what a stuck detector has to stay clear of.
int longest_natural_run(double sigma, double resolution) {
  Noise noise(5);
  double previous = 1e300;
  int run = 1, longest = 1;
  for (int i = 0; i < 100000; ++i) {
    const double reading = std::round((5.0 + sigma * noise.gaussian()) / resolution) * resolution;
    if (reading == previous) {
      ++run;
      if (run > longest) longest = run;
    } else {
      run = 1;
    }
    previous = reading;
  }
  return longest;
}

}  // namespace

RC_TEST("a channel corrects, filters and stamps in one place") {
  Channel channel = plain_channel(1);
  RC_CHECK(channel.health(0) == Health::no_data);

  RC_CHECK(channel.submit(raw_for(5.0), 100 * kMillisecond));
  RC_CHECK_NEAR(channel.reading().value, 5.0, 1e-9);
  RC_CHECK_EQ(channel.reading().sampled_at, 100 * kMillisecond);
  RC_CHECK(channel.reading().valid);
  RC_CHECK(channel.health(120 * kMillisecond) == Health::ok);
}

RC_TEST("a reading outside the physics is refused, and changes nothing") {
  Channel channel = plain_channel(1);
  RC_CHECK(channel.submit(raw_for(5.0), 100 * kMillisecond));

  // A dropout reporting zero counts. Calibrated that is -0.43 m, which is not
  // a distance, so the channel keeps what it had.
  RC_CHECK(!channel.submit(0.0, 110 * kMillisecond));
  RC_CHECK_NEAR(channel.reading().value, 5.0, 1e-9);
  RC_CHECK_EQ(channel.reading().sampled_at, 100 * kMillisecond);
  RC_CHECK_EQ(channel.rejected(), 1);
  RC_CHECK_EQ(channel.received(), 1);

  // A nan is refused by the same test, because it is phrased as the
  // requirement. Written the other way round it would be accepted.
  RC_CHECK(!channel.submit(std::numeric_limits<double>::quiet_NaN(), 120 * kMillisecond));
  RC_CHECK_EQ(channel.rejected(), 2);
  RC_CHECK(!std::isnan(channel.reading().value));

  // And refusals do not keep the channel alive. It goes stale on its own, which
  // is the honest thing for it to say.
  RC_CHECK(channel.health(120 * kMillisecond) == Health::ok);
  RC_CHECK(channel.health(200 * kMillisecond) == Health::stale);
}

RC_TEST("checking after the filter instead of before it") {
  const double truth = 5.0;
  Channel checked = plain_channel(16);
  MovingAverage filtered_first(16);
  const Calibration fit = correction();

  double worst_checked = 0.0, worst_filtered = 0.0;
  int rejected_filtered = 0, spoilt = 0;

  std::cout << "\n    one dropout reading in a steady stream at 5 m\n\n";
  std::cout << "    " << std::right << std::setw(6) << "step" << std::setw(12)
            << "raw" << std::setw(18) << "checked first" << std::setw(18)
            << "filtered first" << "\n";

  for (int i = 0; i < 60; ++i) {
    const double raw = (i == 20) ? 0.0 : raw_for(truth);
    const Nanoseconds when = static_cast<Nanoseconds>(i) * kSampleInterval;

    checked.submit(raw, when);
    const double out_checked = checked.reading().value;

    // The same pipeline with the check moved after the filter, which is where
    // it goes when somebody adds it later.
    const double out_filtered = fit.apply(filtered_first.update(raw));
    if (!(out_filtered >= 0.0 && out_filtered <= 30.0)) ++rejected_filtered;
    if (i > 20 && std::fabs(out_filtered - truth) > 1e-9) ++spoilt;

    worst_checked = std::max(worst_checked, std::fabs(out_checked - truth));
    worst_filtered = std::max(worst_filtered, std::fabs(out_filtered - truth));

    if (i >= 19 && i <= 22)
      std::cout << "    " << std::right << std::setw(6) << i << std::fixed
                << std::setprecision(4) << std::setw(12) << raw << std::setw(18)
                << out_checked << std::setw(18) << out_filtered << "\n";
  }

  std::cout << "\n    " << std::left << std::setw(22) << "refused the dropout"
            << std::right << std::setw(10) << checked.rejected() << std::setw(18)
            << rejected_filtered << "\n";
  std::cout << "    " << std::left << std::setw(22) << "worst error" << std::right
            << std::fixed << std::setprecision(4) << std::setw(10) << worst_checked
            << std::setw(18) << worst_filtered << "\n";
  std::cout << "    " << std::left << std::setw(22) << "outputs spoilt after it"
            << std::right << std::setw(10) << 0 << std::setw(18) << spoilt << "\n";

  std::cout << "\n    the dropout is " << std::setprecision(4) << fit.apply(0.0)
            << " m calibrated, which is outside the range;\n";
  std::cout << "    averaged over 16 it lands well inside it, so the check that\n";
  std::cout << "    comes after the filter can never fire\n";

  RC_CHECK_EQ(checked.rejected(), 1);
  RC_CHECK_EQ(rejected_filtered, 0);
  RC_CHECK(worst_checked < 1e-9);
  RC_CHECK(worst_filtered > 0.3);
  RC_CHECK_EQ(spoilt, 15);
}

RC_TEST("calibrating and filtering commute, and checking does not") {
  MovingAverage a(16), b(16);
  const Calibration fit = correction();
  double worst = 0.0;

  for (int i = 0; i < 500; ++i) {
    const double raw = raw_for(i * 0.01) + ((i % 7) - 3) * 0.01;
    const double calibrate_then_filter = a.update(fit.apply(raw));
    const double filter_then_calibrate = fit.apply(b.update(raw));
    worst = std::max(worst, std::fabs(calibrate_then_filter - filter_then_calibrate));
  }

  std::cout << "\n    calibrate then filter, against filter then calibrate,\n";
  std::cout << "    over 500 readings: worst difference " << std::scientific
            << std::setprecision(1) << worst << "\n"
            << std::fixed;

  // A straight line through an average is the average of a straight line, so
  // these two orders differ only by double rounding. The order that is forced
  // is the check, and it is forced by the test above rather than by algebra.
  RC_CHECK(worst < 1e-12);
}

RC_TEST("a sensor that freezes, stamped two ways") {
  // The device stops sampling at step 50 and the driver keeps handing back the
  // last value it saw. Two channels, differing only in what they are told about
  // when the reading was taken.
  Channel by_device(correction(), 1, 0.050, 0.0, 30.0, 0);
  Channel by_arrival(correction(), 1, 0.050, 0.0, 30.0, 0);
  Nanoseconds frozen_at = 0;

  for (int i = 0; i < 200; ++i) {
    const Nanoseconds now = static_cast<Nanoseconds>(i) * kSampleInterval;
    const bool alive = i < 50;
    if (alive) frozen_at = now;

    by_device.submit(raw_for(5.0), frozen_at);  // the device's own timestamp
    by_arrival.submit(raw_for(5.0), now);       // stamped when it reached us
  }

  const Nanoseconds end = 199 * kSampleInterval;

  // The device's timestamp stopped advancing, so the channel says so.
  RC_CHECK(by_device.health(end) == Health::stale);

  // Stamped on arrival, a dead sensor looks perfectly alive for ever. This is
  // the cost of the shortcut in lesson 15-03: a timestamp you generated cannot
  // tell you anything about a device that stopped.
  RC_CHECK(by_arrival.health(end) == Health::ok);
}

RC_TEST("the repeat counter catches what a generated timestamp cannot") {
  Channel watched(correction(), 1, 0.050, 0.0, 30.0, 5);

  for (int i = 0; i < 4; ++i)
    watched.submit(raw_for(5.0), static_cast<Nanoseconds>(i) * kSampleInterval);
  RC_CHECK(watched.health(4 * kSampleInterval) == Health::ok);

  for (int i = 4; i < 20; ++i)
    watched.submit(raw_for(5.0), static_cast<Nanoseconds>(i) * kSampleInterval);
  RC_CHECK(watched.health(20 * kSampleInterval) == Health::stuck);

  // One different reading and it is healthy again, which is what you want: this
  // is a live judgement, not a latch.
  watched.submit(raw_for(5.1), 21 * kSampleInterval);
  RC_CHECK(watched.health(21 * kSampleInterval) == Health::ok);

  // Stale beats stuck. A channel that has gone quiet is old either way, and
  // stale is the more useful thing for a caller to hear.
  Channel quiet(correction(), 1, 0.050, 0.0, 30.0, 5);
  for (int i = 0; i < 20; ++i)
    quiet.submit(raw_for(5.0), static_cast<Nanoseconds>(i) * kSampleInterval);
  RC_CHECK(quiet.health(20 * kSampleInterval) == Health::stuck);
  RC_CHECK(quiet.health(500 * kSampleInterval) == Health::stale);
}

RC_TEST("how long a healthy parked sensor repeats itself") {
  std::cout << "\n    longest run of identical readings from a parked sensor,\n";
  std::cout << "    100000 samples, resolution 1 mm\n\n";
  std::cout << "    " << std::right << std::setw(12) << "noise" << std::setw(22)
            << "noise / resolution" << std::setw(16) << "longest run" << "\n";

  int run_at_noisy = 0, run_at_quiet = 0;
  for (const double sigma : {0.0500, 0.0100, 0.0020, 0.0010, 0.0005, 0.0001}) {
    const int longest = longest_natural_run(sigma, 0.001);
    if (sigma == 0.0500) run_at_noisy = longest;
    if (sigma == 0.0001) run_at_quiet = longest;
    std::cout << "    " << std::right << std::fixed << std::setprecision(4)
              << std::setw(12) << sigma << std::setprecision(1) << std::setw(22)
              << sigma / 0.001 << std::setw(16) << longest << "\n";
  }

  std::cout << "\n    below its own resolution a parked sensor repeats for ever\n";
  std::cout << "    and is perfectly healthy, so this threshold comes from the\n";
  std::cout << "    sensor rather than from a habit\n";

  // Fifty times the resolution and the longest natural run in a hundred
  // thousand samples is three. A threshold of five is safe there.
  RC_CHECK(run_at_noisy <= 4);

  // A tenth of it and the detector is worthless: it would call a healthy parked
  // sensor stuck within a fraction of a second.
  RC_CHECK(run_at_quiet > 1000);
}

RC_TEST("a channel reports its own lag") {
  Channel channel = plain_channel(64);

  // With nothing received yet, the filter's own lag is all there is to say.
  RC_CHECK_NEAR(channel.lag_seconds(0, kSeconds), 31.5 * kSeconds, 1e-12);

  // A reading taken 20 ms ago, through a filter of 64 at 500 Hz: 63 ms of
  // filter plus 20 ms of transport.
  channel.submit(raw_for(5.0), 100 * kMillisecond);
  RC_CHECK_NEAR(channel.lag_seconds(120 * kMillisecond, kSeconds), 0.063 + 0.020, 1e-12);

  // Which is the number the loop actually cares about, and neither half of it
  // is visible from the reading alone.
  Channel short_window = plain_channel(4);
  short_window.submit(raw_for(5.0), 100 * kMillisecond);
  RC_CHECK(short_window.lag_seconds(120 * kMillisecond, kSeconds) <
           channel.lag_seconds(120 * kMillisecond, kSeconds));
}

RC_TEST("the whole phase, on one sensor") {
  // Everything this phase has dealt with, at once: a sensor that is 1.5 percent
  // short and 42 cm long, jitters by 5 cm, drops out one reading in two
  // hundred, and is 20 ms behind. The robot drives from 2 m to 22 m at 1 m/s.
  const double speed = 1.0;
  const int late = 10;
  const int window = 16;
  Noise noise(21);

  Channel channel(correction(), window, 0.050, 0.0, 30.0, 8);

  std::vector<double> history;
  double truth = 2.0;
  double raw_squared = 0.0, channel_squared = 0.0;
  double transport_squared = 0.0, whole_squared = 0.0;
  int counted = 0, dropouts = 0;

  for (int i = 0; i < 10000; ++i) {
    truth += speed * kSeconds;
    history.push_back(truth);
    if (i < late) continue;

    const double sampled = history[history.size() - 1 - static_cast<std::size_t>(late)];
    const bool dropout = (i % 200) == 0;
    if (dropout) ++dropouts;
    const double raw = dropout ? 0.0 : raw_for(sampled) + 0.05 * noise.gaussian();

    const Nanoseconds when = static_cast<Nanoseconds>(i - late) * kSampleInterval;
    const Nanoseconds now = static_cast<Nanoseconds>(i) * kSampleInterval;
    channel.submit(raw, when);

    // Carried forward by the age of the reading, which corrects the transport
    // and nothing else.
    const double by_age =
        rc::sensor::carried_forward(channel.reading(), speed, now).value;

    // Carried forward by the lag the channel reports, which is the transport
    // and the filter together. The timestamp on a filtered reading is not the
    // moment that reading describes: the average is centred half a window
    // earlier than its newest sample, and only the channel knows that.
    const double by_lag =
        channel.reading().value + speed * channel.lag_seconds(now, kSeconds);

    if (!dropout) {
      const double raw_error = raw - truth;
      raw_squared += raw_error * raw_error;
      ++counted;
    }
    const double channel_error = channel.reading().value - truth;
    channel_squared += channel_error * channel_error;
    transport_squared += (by_age - truth) * (by_age - truth);
    whole_squared += (by_lag - truth) * (by_lag - truth);
  }

  const double raw_rms = std::sqrt(raw_squared / counted);
  const double channel_rms = std::sqrt(channel_squared / counted);
  const double transport_rms = std::sqrt(transport_squared / counted);
  const double whole_rms = std::sqrt(whole_squared / counted);

  std::cout << "\n    one sensor, driving 2 m to 22 m at 1 m/s, " << dropouts
            << " dropouts\n\n";
  std::cout << "    " << std::left << std::setw(34) << "raw, believed to be about now"
            << std::right << std::fixed << std::setprecision(4) << raw_rms << " m\n";
  std::cout << "    " << std::left << std::setw(34) << "checked, calibrated, filtered"
            << std::right << channel_rms << " m\n";
  std::cout << "    " << std::left << std::setw(34) << "carried forward by its age"
            << std::right << transport_rms << " m\n";
  std::cout << "    " << std::left << std::setw(34) << "carried forward by the whole lag"
            << std::right << whole_rms << " m\n";

  const double lag = channel.lag_seconds(9999 * kSampleInterval, kSeconds);
  std::cout << "\n    " << std::left << std::setw(34) << "lag this channel adds"
            << std::right << lag << " s\n";
  std::cout << "    " << std::left << std::setw(34) << "  the filter"
            << std::right << MovingAverage::lag_samples(window) * kSeconds << " s\n";
  std::cout << "    " << std::left << std::setw(34) << "  the transport"
            << std::right << late * kSeconds << " s\n";

  // Every dropout was refused, and nothing else was.
  RC_CHECK_EQ(channel.rejected(), dropouts);

  // Each step is better than the one above it, and the last one needs the
  // channel to know its own lag.
  RC_CHECK(channel_rms < raw_rms);
  RC_CHECK(transport_rms < channel_rms);
  RC_CHECK(whole_rms < transport_rms);

  // And the cost is a number rather than a guess.
  RC_CHECK_NEAR(lag, 0.015 + 0.020, 1e-9);
}
