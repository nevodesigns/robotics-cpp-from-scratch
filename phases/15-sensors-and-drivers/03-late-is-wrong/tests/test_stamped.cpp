#include <rc/test/rc_test.hpp>

#include <rc/control/pid.hpp>
#include <rc/control/tuning.hpp>
#include <rc/core/clock.hpp>
#include <rc/core/filters.hpp>

#include <cmath>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

#include "solution.hpp"

namespace {

using rc::core::Nanoseconds;

constexpr double kDt = 0.002;
constexpr int kSteps = 8000;
constexpr Nanoseconds kMillisecond = 1000000;

std::string seconds_or_never(double value) {
  if (value < 0.0) return "never";
  std::ostringstream out;
  out << std::fixed << std::setprecision(2) << value;
  return out.str();
}

// Uniform values that are the same on every compiler, for the same reason as in
// lesson 15-02: a table nobody can reproduce is not a measurement.
class Jitter {
 public:
  explicit Jitter(std::uint64_t seed) : state_(seed * 6364136223846793005ULL + 1ULL) {}

  // A whole number of steps in [low, high].
  int steps(int low, int high) {
    state_ = state_ * 6364136223846793005ULL + 1442695040888963407ULL;
    const double unit = static_cast<double>((state_ >> 11) & 0x1FFFFFFFFFFFFFULL) /
                        static_cast<double>(0x20000000000000ULL);
    return low + static_cast<int>(unit * static_cast<double>(high - low + 1));
  }

 private:
  std::uint64_t state_;
};

struct Run {
  rc::control::StepResponse response;
  double effort = 0.0;
};

// The loop from phase 14, with two things done to its measurement: it can be
// delayed by a whole number of steps, and it can be averaged over a window.
// Both of those are lag, and the question this lesson asks is whether the loop
// can tell them apart.
Run loop_with(int late, int window) {
  rc::control::Pid pid(20.0, 0.0, 8.0, -20.0, 20.0);
  rc::control::Mass plant(1.0, 0.6, 0.0);
  rc::core::MovingAverage filter(window < 1 ? 1 : window);

  std::vector<double> pipe(static_cast<std::size_t>(late > 0 ? late : 0), 0.0);
  std::size_t next = 0;
  std::vector<double> output;
  output.reserve(kSteps);
  double sum_squared = 0.0;

  for (int i = 0; i < kSteps; ++i) {
    double arrived = plant.position();
    if (late > 0) {
      const double oldest = pipe[next];
      pipe[next] = arrived;
      next = (next + 1) % pipe.size();
      arrived = oldest;
    }
    const double measured = window > 1 ? filter.update(arrived) : arrived;
    const double command = pid.update(1.0, measured, kDt);
    sum_squared += command * command;
    output.push_back(plant.step(command, kDt));
  }

  Run run;
  run.response = rc::control::analyse(output, 1.0, kDt, 0.02);
  run.effort = std::sqrt(sum_squared / kSteps);
  return run;
}

// A robot driving in a straight line at a steady speed, watched by a sensor
// that is `late` steps behind. Returns the worst error the consumer sees,
// having believed the reading was about now.
double worst_error_at(double speed, int late) {
  std::vector<double> history;
  double truth = 0.0, worst = 0.0;
  for (int i = 0; i < 2000; ++i) {
    truth += speed * kDt;
    history.push_back(truth);
    if (i < late) continue;
    const double arrived = history[history.size() - 1 - static_cast<std::size_t>(late)];
    worst = std::max(worst, std::fabs(arrived - truth));
  }
  return worst;
}

}  // namespace

RC_TEST("the age of a reading is a difference between two times, and may be negative") {
  Stamped<double> reading;
  reading.value = 4.0;
  reading.sampled_at = 100 * kMillisecond;
  reading.valid = true;

  RC_CHECK_NEAR(age_seconds(reading, 100 * kMillisecond), 0.0, 1e-12);
  RC_CHECK_NEAR(age_seconds(reading, 120 * kMillisecond), 0.020, 1e-12);
  RC_CHECK_NEAR(age_seconds(reading, 1100 * kMillisecond), 1.0, 1e-12);

  // A device with its own oscillator, stamping in its own time, hands you this
  // one day. Hiding it behind a clamp turns a clock problem into a mystery.
  RC_CHECK_NEAR(age_seconds(reading, 80 * kMillisecond), -0.020, 1e-12);
}

RC_TEST("freshness is a requirement that is negated, not a list of rejections") {
  Stamped<double> reading;
  reading.value = 4.0;
  reading.sampled_at = 100 * kMillisecond;
  reading.valid = true;

  RC_CHECK(fresh(reading, 120 * kMillisecond, 0.050));
  RC_CHECK(fresh(reading, 150 * kMillisecond, 0.050));   // exactly at the limit
  RC_CHECK(!fresh(reading, 151 * kMillisecond, 0.050));  // and just past it

  // A reading that never arrived is never fresh, whatever its timestamp says.
  Stamped<double> never;
  never.sampled_at = 100 * kMillisecond;
  RC_CHECK(!fresh(never, 100 * kMillisecond, 0.050));

  // Stamped in the future. Two clocks disagree, and acting on it would be
  // acting on a reading that has not happened.
  RC_CHECK(!fresh(reading, 80 * kMillisecond, 0.050));

  // And the phrasing that E-SENSE-0007 exists for. A nan limit rejects rather
  // than accepts, which is what happens when the condition is the requirement.
  const double nan_limit = std::numeric_limits<double>::quiet_NaN();
  RC_CHECK(!fresh(reading, 120 * kMillisecond, nan_limit));
}

RC_TEST("carrying a reading forward moves its timestamp with it") {
  Stamped<double> reading;
  reading.value = 2.0;
  reading.sampled_at = 100 * kMillisecond;
  reading.valid = true;

  const Stamped<double> now = carried_forward(reading, 1.5, 140 * kMillisecond);
  RC_CHECK_NEAR(now.value, 2.06, 1e-12);   // 40 ms at 1.5 m/s
  RC_CHECK_EQ(now.sampled_at, 140 * kMillisecond);
  RC_CHECK(now.valid);

  // Carried forward to the moment it was taken, it is itself.
  RC_CHECK_NEAR(carried_forward(reading, 1.5, 100 * kMillisecond).value, 2.0, 1e-12);

  // There is nothing to carry forward about a reading that never arrived, and
  // inventing a value for it would be worse than admitting there is none.
  Stamped<double> never;
  never.value = 99.0;
  RC_CHECK(!carried_forward(never, 1.5, 500 * kMillisecond).valid);
}

RC_TEST("a loop steering by a late measurement") {
  std::cout << "\n    the loop from 14-04, with nothing changed but how old\n";
  std::cout << "    its measurement is\n\n";
  std::cout << "    " << std::right << std::setw(8) << "late" << std::setw(11)
            << "seconds" << std::setw(13) << "overshoot" << std::setw(11)
            << "settles" << std::setw(11) << "effort" << "\n";

  Run gentle = loop_with(0, 1), harsh = loop_with(0, 1);
  for (const int late : {0, 5, 15, 30, 60, 120, 250}) {
    const Run run = loop_with(late, 1);
    if (late == 30) gentle = run;
    if (late == 120) harsh = run;
    std::cout << "    " << std::right << std::setw(8) << late << std::fixed
              << std::setprecision(3) << std::setw(11) << late * kDt
              << std::setprecision(1) << std::setw(12) << run.response.overshoot * 100.0
              << "%" << std::setw(11) << seconds_or_never(run.response.settling_time)
              << std::setprecision(2) << std::setw(11) << run.effort << "\n";
  }

  // Sixty milliseconds of latency is free. A loop that settles in 1.2 seconds
  // does not notice it at all.
  RC_CHECK(gentle.response.settling_time > 0.0);
  RC_CHECK(gentle.response.overshoot < 0.01);

  // A quarter of a second of it, and the same loop with the same gains
  // overshoots by ninety eight percent and never settles.
  RC_CHECK(harsh.response.settling_time < 0.0);
  RC_CHECK(harsh.response.overshoot > 0.9);
  RC_CHECK(harsh.effort > gentle.effort * 5.0);
}

RC_TEST("the loop cannot tell where the lag came from") {
  std::cout << "\n    the same lag, bought two ways\n\n";
  std::cout << "    " << std::left << std::setw(16) << "source" << std::right
            << std::setw(10) << "lag s" << std::setw(13) << "overshoot"
            << std::setw(11) << "settles" << "\n";

  for (const int window : {64, 128, 256}) {
    const int equivalent = (window - 1) / 2;
    const Run filtered = loop_with(0, window);
    const Run delayed = loop_with(equivalent, 1);

    std::cout << "    " << std::left << std::setw(16)
              << ("a filter of " + std::to_string(window)) << std::right << std::fixed
              << std::setprecision(4) << std::setw(10) << equivalent * kDt
              << std::setprecision(1) << std::setw(12)
              << filtered.response.overshoot * 100.0 << "%" << std::setw(11)
              << seconds_or_never(filtered.response.settling_time) << "\n";
    std::cout << "    " << std::left << std::setw(16)
              << ("a bus " + std::to_string(equivalent) + " late") << std::right
              << std::fixed << std::setprecision(4) << std::setw(10) << equivalent * kDt
              << std::setprecision(1) << std::setw(12)
              << delayed.response.overshoot * 100.0 << "%" << std::setw(11)
              << seconds_or_never(delayed.response.settling_time) << "\n";

    // Same lag, same verdict. A filter is very slightly gentler because it also
    // attenuates, but the difference is nothing next to the agreement.
    if (filtered.response.settling_time > 0.0 || delayed.response.settling_time > 0.0) {
      RC_CHECK(std::fabs(filtered.response.overshoot - delayed.response.overshoot) < 0.05);
    } else {
      RC_CHECK(filtered.response.overshoot > 0.9);
      RC_CHECK(delayed.response.overshoot > 0.9);
    }
  }

  std::cout << "\n    one of those two numbers is written in your code and one\n";
  std::cout << "    of them is not written anywhere\n";
}

RC_TEST("the error a late reading causes is speed times latency, exactly") {
  std::cout << "\n    a sensor 20 ms behind\n\n";
  std::cout << "    " << std::right << std::setw(11) << "speed m/s" << std::setw(16)
            << "worst error" << std::setw(18) << "speed x 0.020 s" << "\n";

  const int late = 10;  // 20 ms at 2 ms per step
  for (const double speed : {0.0, 0.1, 0.5, 1.0, 2.0, 4.0}) {
    const double worst = worst_error_at(speed, late);
    std::cout << "    " << std::right << std::fixed << std::setprecision(2)
              << std::setw(11) << speed << std::setprecision(4) << std::setw(16)
              << worst << std::setw(18) << speed * late * kDt << "\n";
    RC_CHECK_NEAR(worst, speed * late * kDt, 1e-9);
  }

  // Which is zero standing still. Every latency bug in this curriculum's atlas
  // passes its bench test for this reason and appears the first time the robot
  // moves at speed.
  RC_CHECK_EQ(worst_error_at(0.0, late), 0.0);
  RC_CHECK_EQ(worst_error_at(0.0, 500), 0.0);
}

RC_TEST("latency that jitters looks like sensor noise and is not") {
  const double speed = 1.0;
  const int settle = 40;
  const int samples = 20000;

  Jitter jitter(11);
  std::vector<double> history;
  double truth = 0.0;
  double naive_squared = 0.0, naive_worst = 0.0;
  double stamped_squared = 0.0, stamped_worst = 0.0;
  rc::core::MovingAverage smoother(16);
  double smoothed_squared = 0.0;
  int counted = 0;

  for (int i = 0; i < samples; ++i) {
    truth += speed * kDt;
    history.push_back(truth);

    const int late = jitter.steps(2, 18);  // 4 ms to 36 ms
    if (i < settle) continue;

    const double arrived = history[history.size() - 1 - static_cast<std::size_t>(late)];

    // Believed to be about now.
    const double naive = arrived - truth;
    naive_squared += naive * naive;
    naive_worst = std::max(naive_worst, std::fabs(naive));

    // Smoothing it, which is what everybody tries first.
    const double smoothed = smoother.update(arrived) - truth;
    smoothed_squared += smoothed * smoothed;

    // Carried forward from the moment it was taken.
    Stamped<double> reading;
    reading.value = arrived;
    reading.sampled_at = static_cast<Nanoseconds>(i - late) * 2 * kMillisecond;
    reading.valid = true;
    const double corrected =
        carried_forward(reading, speed, static_cast<Nanoseconds>(i) * 2 * kMillisecond).value -
        truth;
    stamped_squared += corrected * corrected;
    stamped_worst = std::max(stamped_worst, std::fabs(corrected));

    ++counted;
  }

  const double naive_rms = std::sqrt(naive_squared / counted);
  const double smoothed_rms = std::sqrt(smoothed_squared / counted);
  const double stamped_rms = std::sqrt(stamped_squared / counted);

  std::cout << "\n    latency jittering between 4 and 36 ms, driving at 1 m/s\n\n";
  std::cout << "    " << std::left << std::setw(26) << "" << std::right
            << std::setw(12) << "rms" << std::setw(12) << "worst" << "\n";
  std::cout << "    " << std::left << std::setw(26) << "believed to be about now"
            << std::right << std::fixed << std::setprecision(4) << std::setw(12)
            << naive_rms << std::setw(12) << naive_worst << "\n";
  std::cout << "    " << std::left << std::setw(26) << "and then smoothed over 16"
            << std::right << std::setw(12) << smoothed_rms << "\n";
  std::cout << "    " << std::left << std::setw(26) << "carried forward instead"
            << std::right << std::setw(12) << stamped_rms << std::setw(12)
            << stamped_worst << "\n";

  // Two centimetres of error that is not noise, at one metre per second.
  RC_CHECK(naive_rms > 0.015);

  // Smoothing makes it worse, which is the part worth stopping over, because
  // smoothing is what everybody reaches for. The error is not zero mean noise
  // on the reading, it is the reading being about the wrong moment, and the
  // average of a set of wrong moments is an older wrong moment: the filter adds
  // its own 15 ms of lag on top of the 20 the bus already cost.
  RC_CHECK(smoothed_rms > naive_rms);

  // The timestamp removes it entirely, because it was never noise.
  RC_CHECK(stamped_rms < naive_rms * 0.01);
}

RC_TEST("carrying forward at a rate you only half know is still most of the fix") {
  const double speed = 1.0;
  const int late = 10;

  std::cout << "\n    the rate used to carry the reading forward is itself an\n";
  std::cout << "    estimate, so how wrong may it be\n\n";
  std::cout << "    " << std::right << std::setw(16) << "rate error" << std::setw(14)
            << "rms left" << std::setw(20) << "of the 0.0200 m" << "\n";

  for (const double wrong : {0.0, 0.10, 0.25, 0.50, 1.00}) {
    std::vector<double> history;
    double truth = 0.0, squared = 0.0;
    int counted = 0;
    for (int i = 0; i < 20000; ++i) {
      truth += speed * kDt;
      history.push_back(truth);
      if (i < late) continue;

      Stamped<double> reading;
      reading.value = history[history.size() - 1 - static_cast<std::size_t>(late)];
      reading.sampled_at = static_cast<Nanoseconds>(i - late) * 2 * kMillisecond;
      reading.valid = true;

      const double error =
          carried_forward(reading, speed * (1.0 - wrong),
                          static_cast<Nanoseconds>(i) * 2 * kMillisecond)
              .value -
          truth;
      squared += error * error;
      ++counted;
    }
    const double rms = std::sqrt(squared / counted);
    std::cout << "    " << std::right << std::fixed << std::setprecision(0)
              << std::setw(15) << wrong * 100.0 << "%" << std::setprecision(4)
              << std::setw(14) << rms << std::setprecision(2) << std::setw(19)
              << rms / 0.02 * 100.0 << "%" << "\n";

    // What is left is the original error times how wrong the rate was, so an
    // estimate that is half wrong still removes half the problem, and a rate of
    // zero is exactly not compensating at all.
    RC_CHECK_NEAR(rms, 0.02 * wrong, 1e-9);
  }
}
