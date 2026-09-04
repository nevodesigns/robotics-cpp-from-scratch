#include <rc/test/rc_test.hpp>

#include <rc/control/pid.hpp>
#include <rc/control/tuning.hpp>

#include <cmath>
#include <iomanip>
#include <iostream>
#include <random>
#include <string>
#include <vector>

#include "solution.hpp"

namespace {

constexpr double kDt = 0.002;
constexpr int kSteps = 4000;

std::string seconds_or_never(double value) {
  if (value < 0.0) return "never";
  std::ostringstream out;
  out << std::fixed << std::setprecision(2) << value;
  return out.str();
}

// How much of the input noise is left after filtering, as a fraction.
double surviving_noise(int window, unsigned seed) {
  MovingAverage filter(window);
  std::mt19937 rng(seed);
  std::normal_distribution<double> noise(0.0, 0.05);

  // Warm it up first, so this measures the filter and not its first N readings.
  for (int i = 0; i < window; ++i) filter.update(noise(rng));

  double sum_squared = 0.0;
  const int samples = 20000;
  for (int i = 0; i < samples; ++i) {
    const double out = filter.update(noise(rng));
    sum_squared += out * out;
  }
  return std::sqrt(sum_squared / samples) / 0.05;
}

// How many readings after a step before the output passes halfway.
int lag_to_half(int window) {
  MovingAverage filter(window);
  for (int i = 0; i < window * 2; ++i) filter.update(0.0);
  for (int i = 0; i < window * 4; ++i)
    if (filter.update(1.0) >= 0.5) return i;
  return -1;
}

// A controller steering by a filtered measurement of a noisy sensor.
rc::control::StepResponse loop_with(int window, double* command_effort) {
  rc::control::Pid pid(20.0, 0.0, 8.0, -20.0, 20.0);
  rc::control::Mass plant(1.0, 0.6, 0.0);
  MovingAverage filter(window);

  std::mt19937 rng(9);
  std::normal_distribution<double> sensor(0.0, 0.01);

  std::vector<double> output;
  output.reserve(static_cast<std::size_t>(kSteps));
  double effort = 0.0;

  for (int i = 0; i < kSteps; ++i) {
    const double measured = filter.update(plant.position() + sensor(rng));
    const double command = pid.update(1.0, measured, kDt);
    effort += command * command;
    output.push_back(plant.step(command, kDt));
  }
  if (command_effort != nullptr) *command_effort = std::sqrt(effort / kSteps);

  return rc::control::analyse(rc::span<const double>(output.data(), output.size()),
                              1.0, kDt, 0.02);
}

}  // namespace

RC_TEST("a window of one is not a filter") {
  MovingAverage filter(1);
  RC_CHECK_NEAR(filter.update(3.0), 3.0, 1e-12);
  RC_CHECK_NEAR(filter.update(-7.0), -7.0, 1e-12);
  RC_CHECK(filter.warm());
}

RC_TEST("a window smaller than one is refused rather than dividing by zero") {
  MovingAverage filter(0);
  RC_CHECK_EQ(filter.window(), 1);
  RC_CHECK(std::isfinite(filter.update(5.0)));
}

RC_TEST("the average is over the readings there are, not the window there will be") {
  // The check that catches a warm up transient. A filter that divides by its
  // window before the window is full reports a fraction of the truth and
  // climbs to it, so a robot switched on next to a wall believes the wall is
  // far away.
  MovingAverage filter(10);
  RC_CHECK_NEAR(filter.update(4.0), 4.0, 1e-12);
  RC_CHECK_NEAR(filter.update(4.0), 4.0, 1e-12);
  RC_CHECK_NEAR(filter.update(4.0), 4.0, 1e-12);
  RC_CHECK(!filter.warm());

  for (int i = 0; i < 7; ++i) filter.update(4.0);
  RC_CHECK(filter.warm());
  RC_CHECK_NEAR(filter.update(4.0), 4.0, 1e-12);
}

RC_TEST("the oldest reading leaves the window") {
  // A filter that never drops anything is a running mean over all of history,
  // which converges on the average of the whole run and stops responding.
  MovingAverage filter(3);
  filter.update(0.0);
  filter.update(0.0);
  filter.update(0.0);
  RC_CHECK_NEAR(filter.update(3.0), 1.0, 1e-12);
  RC_CHECK_NEAR(filter.update(3.0), 2.0, 1e-12);
  RC_CHECK_NEAR(filter.update(3.0), 3.0, 1e-12);
}

RC_TEST("a constant signal passes through unchanged, whatever the window") {
  for (const int window : {1, 4, 32, 128}) {
    MovingAverage filter(window);
    for (int i = 0; i < window * 2; ++i) filter.update(2.5);
    RC_CHECK_NEAR(filter.update(2.5), 2.5, 1e-12);
  }
}

RC_TEST("what a window buys and what it costs, measured") {
  // Both numbers are worth knowing exactly, because the whole design decision
  // is a trade between them.
  std::cout << "\n    " << std::left << std::setw(10) << "window" << std::right
            << std::setw(14) << "noise left" << std::setw(14) << "predicted"
            << std::setw(12) << "lag steps" << std::setw(12) << "predicted" << "\n";

  for (const int window : {1, 4, 16, 64, 256}) {
    const double measured_noise = surviving_noise(window, 5);
    const int measured_lag = lag_to_half(window);

    std::cout << "    " << std::left << std::setw(10) << window << std::right
              << std::fixed << std::setprecision(3) << std::setw(14) << measured_noise
              << std::setw(14) << MovingAverage::noise_gain(window)
              << std::setw(12) << measured_lag
              << std::setw(12) << std::setprecision(1)
              << MovingAverage::lag_samples(window) << "\n";

    // Noise falls as one over the square root of the window.
    RC_CHECK_NEAR(measured_noise, MovingAverage::noise_gain(window), 0.05);

    // And the lag is the average age of what is in the window.
    RC_CHECK_NEAR(static_cast<double>(measured_lag),
                  MovingAverage::lag_samples(window), 1.0);
  }
  std::cout << "\n";
}

RC_TEST("too little filtering is an unstable loop") {
  // The check that catches leaving a noisy measurement unfiltered. The
  // derivative term differentiates the noise along with the signal, and the
  // command spends its life at the limits.
  double raw_effort = 0.0;
  double filtered_effort = 0.0;
  const rc::control::StepResponse raw = loop_with(1, &raw_effort);
  const rc::control::StepResponse filtered = loop_with(16, &filtered_effort);

  RC_CHECK(raw.settling_time < 0.0);        // never settles
  RC_CHECK(filtered.settling_time > 0.0);   // and with a filter it does
  RC_CHECK(raw_effort > filtered_effort * 3.0);
}

RC_TEST("too much filtering is also an unstable loop") {
  // And the other side of the same trade. The lag of a long window is a delay
  // in the feedback path, and a controller steering by where the robot was is
  // a controller that overshoots and keeps going.
  double effort = 0.0;
  const rc::control::StepResponse gentle = loop_with(64, &effort);
  const rc::control::StepResponse heavy = loop_with(256, &effort);

  RC_REQUIRE(gentle.settling_time > 0.0);
  RC_CHECK(gentle.overshoot < 0.05);

  RC_CHECK(heavy.settling_time < 0.0);
  RC_CHECK(heavy.overshoot > 0.5);
}

RC_TEST("the whole trade, printed") {
  std::cout << "    " << std::left << std::setw(10) << "window" << std::right
            << std::setw(12) << "lag s" << std::setw(13) << "overshoot"
            << std::setw(11) << "settle s" << std::setw(13) << "command rms" << "\n";

  for (const int window : {1, 4, 16, 64, 128, 256, 512}) {
    double effort = 0.0;
    const rc::control::StepResponse response = loop_with(window, &effort);
    std::cout << "    " << std::left << std::setw(10) << window << std::right
              << std::fixed << std::setprecision(4) << std::setw(12)
              << MovingAverage::lag_samples(window) * kDt
              << std::setprecision(1) << std::setw(12) << response.overshoot * 100.0 << "%"
              << std::setw(11) << seconds_or_never(response.settling_time)
              << std::setprecision(2) << std::setw(13) << effort << "\n";
  }
  std::cout << "\n    the loop settles in about 1.2 seconds, and the filter that\n"
               "    ruins it is the one whose lag is a fifth of that\n\n";
  RC_CHECK(true);
}
