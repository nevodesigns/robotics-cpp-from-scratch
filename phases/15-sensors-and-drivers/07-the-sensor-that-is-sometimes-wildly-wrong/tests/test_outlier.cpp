#include <rc/test/rc_test.hpp>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <vector>

#include "solution.hpp"

namespace {

const double kPi = 3.141592653589793;

struct Rng {
  std::uint32_t state;
  std::uint32_t next() {
    state ^= state << 13;
    state ^= state >> 17;
    state ^= state << 5;
    return state;
  }
  double uniform() { return static_cast<double>(next() >> 8) / 16777216.0; }

  // Roughly a normal number: twelve uniform ones add up to a bell curve, with a mean
  // of six and a standard deviation of one.
  double gaussian() {
    double sum = 0.0;
    for (int i = 0; i < 12; ++i) sum += uniform();
    return sum - 6.0;
  }
};

// The median by the plain method, sorting a copy, and with the same rule for an even
// count as the filters state. Nothing here shares code with the filters.
double median_of(std::vector<double> values) {
  std::sort(values.begin(), values.end());
  return values[values.size() / 2];
}

std::vector<double> last_window(const std::vector<double>& history, std::size_t window) {
  const std::size_t start = history.size() > window ? history.size() - window : 0;
  return std::vector<double>(history.begin() + static_cast<long>(start), history.end());
}

double rms(const std::vector<double>& output, const std::vector<double>& truth, std::size_t from, std::size_t lag) {
  double sum = 0.0;
  std::size_t n = 0;
  for (std::size_t i = from + lag; i < output.size(); ++i) {
    const double e = output[i] - truth[i - lag];
    sum += e * e;
    ++n;
  }
  return std::sqrt(sum / static_cast<double>(n));
}

double plain_mean(const std::vector<double>& v) {
  double sum = 0.0;
  for (const double x : v) sum += x;
  return sum / static_cast<double>(v.size());
}

}  // namespace

RC_TEST("the median filter agrees with sorting a copy, while it fills and after, for every window") {
  Rng rng{7};
  std::size_t checked = 0;
  for (const std::size_t window : {1u, 2u, 3u, 5u, 9u, 21u}) {
    MedianFilter filter(window);
    std::vector<double> history;
    for (int i = 0; i < 300; ++i) {
      const double x = std::round(rng.gaussian() * 1000.0) / 100.0;   // repeats happen, which is the awkward case
      history.push_back(x);
      const double expected = median_of(last_window(history, window));
      RC_CHECK_NEAR(filter.update(x), expected, 1e-12);
      ++checked;
    }
    RC_CHECK_EQ(filter.window(), window);
  }
  RC_CHECK_EQ(checked, static_cast<std::size_t>(1800));

  RC_CHECK_EQ(MedianFilter(9).delay_samples(), static_cast<std::size_t>(4));
  RC_CHECK_EQ(MedianFilter(1).delay_samples(), static_cast<std::size_t>(0));
  RC_CHECK_EQ(MedianFilter(0).window(), static_cast<std::size_t>(1));   // a window of nothing is a window of one
}

RC_TEST("a spike moves a mean by its size over the window, and a median not at all") {
  const std::size_t window = 9;
  MedianFilter median(window);
  std::vector<double> history;
  double worst_mean = 0.0;
  double worst_median = 0.0;
  double last = 0.0;

  // A steady reading of 5, and one of 105 in the middle.
  for (int i = 0; i < 40; ++i) {
    const double x = i == 20 ? 105.0 : 5.0;
    history.push_back(x);
    worst_mean = std::fmax(worst_mean, std::fabs(plain_mean(last_window(history, window)) - 5.0));
    last = median.update(x);
    worst_median = std::fmax(worst_median, std::fabs(last - 5.0));
  }

  std::cout << "  one reading of 105 among readings of 5, a window of nine:\n"
            << "    a mean moves by up to " << std::fixed << std::setprecision(1) << worst_mean << " for nine readings\n"
            << "    a median moves by " << worst_median << "\n\n";

  RC_CHECK_NEAR(worst_mean, 100.0 / 9.0, 1e-9);
  RC_CHECK_NEAR(worst_median, 0.0, 1e-12);
  RC_CHECK_NEAR(last, 5.0, 1e-12);   // and it is reporting the reading, not just a small number
}

RC_TEST("a median fails when more than half the window is bad, and not before") {
  const std::size_t window = 9;

  std::cout << "  a burst of consecutive bad readings of 100, a window of nine:\n"
            << "    burst length   worst output\n";
  for (int burst = 1; burst <= 6; ++burst) {
    MedianFilter filter(window);
    double worst = 0.0;
    for (int i = 0; i < 40; ++i) {
      const double x = (i >= 20 && i < 20 + burst) ? 100.0 : 0.0;
      worst = std::fmax(worst, std::fabs(filter.update(x)));
    }
    std::cout << "    " << std::setw(8) << burst << std::setw(14) << std::fixed << std::setprecision(0) << worst << "\n";

    // Up to four of nine are outvoted. The fifth is a majority.
    if (burst <= 4) RC_CHECK_NEAR(worst, 0.0, 1e-12);
    else RC_CHECK_NEAR(worst, 100.0, 1e-12);
  }
  std::cout << "\n  The limit is half the window, rounded down: (9 - 1) / 2 = 4 bad readings in a row.\n\n";
}

RC_TEST("on a step a median stays sharp and a mean smears it over the whole window") {
  const std::size_t window = 9;
  std::vector<double> mean_out;
  std::vector<double> median_out;
  MedianFilter median(window);
  std::vector<double> history;

  for (int i = 0; i < 60; ++i) {
    const double x = i < 30 ? 0.0 : 1.0;
    history.push_back(x);
    mean_out.push_back(plain_mean(last_window(history, window)));
    median_out.push_back(median.update(x));
  }

  const auto crossing = [](const std::vector<double>& y, double level) {
    for (std::size_t i = 0; i < y.size(); ++i) {
      if (y[i] >= level) return static_cast<int>(i);
    }
    return -1;
  };
  const int mean_rise = crossing(mean_out, 0.9) - crossing(mean_out, 0.1);
  const int median_rise = crossing(median_out, 0.9) - crossing(median_out, 0.1);
  const int median_delay = crossing(median_out, 0.5) - 30;

  std::cout << "  a step from 0 to 1, a window of nine, samples from ten percent to ninety:\n"
            << "    a mean takes " << mean_rise << " samples to rise\n"
            << "    a median takes " << median_rise << ", and arrives " << median_delay << " samples late\n\n";

  RC_CHECK_EQ(mean_rise, 8);
  RC_CHECK_EQ(median_rise, 0);
  RC_CHECK_EQ(static_cast<std::size_t>(median_delay), median.delay_samples());
}

RC_TEST("the Hampel filter passes every reading that is not an outlier exactly as it came") {
  Rng rng{11};
  HampelFilter filter(9, 3.0);
  int changed = 0;
  for (int i = 0; i < 2000; ++i) {
    const double x = 5.0 + 0.05 * rng.gaussian();
    if (filter.update(x) != x) ++changed;
  }

  // A window of nine estimates the spread poorly, so more than the three in a thousand
  // that three known deviations would give are replaced, and the rest are not touched at
  // all: not smoothed, not rounded, not shifted.
  std::cout << "  2000 readings of clean noise: " << changed << " replaced, " << filter.rejected() << " counted.\n\n";
  RC_CHECK_EQ(static_cast<std::size_t>(changed), filter.rejected());
  RC_CHECK(changed < 100);
  RC_CHECK(changed > 0);
}

RC_TEST("the Hampel filter agrees with the definition, worked out by sorting") {
  Rng rng{3};
  const std::size_t window = 7;
  HampelFilter filter(window, 3.0, 0.01);
  std::vector<double> history;
  int checked = 0;
  int replaced = 0;

  for (int i = 0; i < 1000; ++i) {
    double x = std::sin(static_cast<double>(i) * 0.05) + 0.05 * rng.gaussian();
    if (rng.uniform() < 0.05) x += 10.0;
    history.push_back(x);

    const std::vector<double> w = last_window(history, window);
    const double m = median_of(w);
    std::vector<double> d;
    for (const double v : w) d.push_back(std::fabs(v - m));
    const double scale = std::max(1.4826 * median_of(d), 0.01);
    const double expected = std::fabs(x - m) > 3.0 * scale ? m : x;
    if (expected != x) ++replaced;

    RC_CHECK_NEAR(filter.update(x), expected, 1e-12);
    ++checked;
  }
  RC_CHECK_EQ(checked, 1000);
  RC_CHECK(replaced > 20);
}

RC_TEST("what each costs, on a slow signal with noise and one reading in a hundred wrong") {
  Rng rng{99};
  const std::size_t n = 20000;
  const std::size_t window = 9;
  std::vector<double> truth(n);
  std::vector<double> raw(n);
  int spikes = 0;

  for (std::size_t i = 0; i < n; ++i) {
    truth[i] = std::sin(2.0 * kPi * static_cast<double>(i) / 200.0);
    raw[i] = truth[i] + 0.05 * rng.gaussian();
    if (rng.uniform() < 0.01) {
      const double side = rng.uniform() < 0.5 ? -1.0 : 1.0;   // one draw at a time: the order of a product's two sides is the compiler's choice
      const double size = 50.0 + 50.0 * rng.uniform();
      raw[i] += side * size;
      ++spikes;
    }
  }

  MedianFilter median(window);
  HampelFilter hampel(window, 3.0);
  std::vector<double> mean_out(n);
  std::vector<double> median_out(n);
  std::vector<double> hampel_out(n);
  std::vector<double> history;
  for (std::size_t i = 0; i < n; ++i) {
    history.push_back(raw[i]);
    mean_out[i] = plain_mean(last_window(history, window));
    median_out[i] = median.update(raw[i]);
    hampel_out[i] = hampel.update(raw[i]);
  }

  const double raw_rms = rms(raw, truth, window, 0);
  const double mean_rms = rms(mean_out, truth, window, 0);
  const double median_rms = rms(median_out, truth, window, 0);
  const double median_aligned = rms(median_out, truth, window, median.delay_samples());
  const double hampel_rms = rms(hampel_out, truth, window, 0);

  std::cout << "  " << spikes << " readings of " << n << " wrong by 50 to 100, on a sine of 1 with noise of 0.05:\n\n"
            << "    filter                       rms error against the truth\n"
            << std::fixed << std::setprecision(3)
            << "    none                         " << std::setw(8) << raw_rms << "\n"
            << "    mean of nine                 " << std::setw(8) << mean_rms << "\n"
            << "    median of nine               " << std::setw(8) << median_rms << "   (" << median_aligned
            << " once its four samples of delay are allowed for)\n"
            << "    Hampel, nine, three sigma    " << std::setw(8) << hampel_rms << "\n\n";

  RC_CHECK(spikes > 150 && spikes < 280);

  // The mean spreads each spike over nine readings and is still wildly wrong. The
  // median is nearly right, and most of what remains is its delay.
  RC_CHECK(raw_rms > 5.0);
  RC_CHECK(mean_rms > 1.0);
  RC_CHECK(median_rms < 0.2);
  RC_CHECK(median_aligned < 0.05);
  RC_CHECK(median_rms < mean_rms / 10.0);

  // Hampel is at the noise floor: it removed the spikes and left everything else alone.
  RC_CHECK(hampel_rms < 0.07);
  RC_CHECK(hampel_rms > 0.04);
}

RC_TEST("a real step looks like a spike until half a window agrees, and a floor on the spread helps a small one") {
  const std::size_t window = 9;

  // A step of 1 on a sensor that was perfectly quiet: how long it is held back.
  HampelFilter strict(window, 3.0, 0.0);
  int held_back = 0;
  for (int i = 0; i < 40; ++i) {
    const double x = i < 20 ? 0.0 : 1.0;
    const double y = strict.update(x);
    if (i >= 20 && y < 0.5) ++held_back;
  }

  // A change of 0.003, three times the sensor's resolution of 0.001, on the same quiet
  // sensor: with no floor on the spread it is rejected, with one it is not.
  HampelFilter without_floor(window, 3.0, 0.0);
  HampelFilter with_floor(window, 3.0, 0.001);
  int rejected_without = 0;
  int rejected_with = 0;
  for (int i = 0; i < 40; ++i) {
    const double x = i < 20 ? 0.0 : 0.003;
    if (without_floor.update(x) != x) ++rejected_without;
    if (with_floor.update(x) != x) ++rejected_with;
  }

  std::cout << "  a step of 1 on a sensor that has been perfectly quiet is held back for " << held_back
            << " readings,\n  until half the window has seen it. A change of 0.003 on the same sensor, at three\n"
            << "  times its resolution of 0.001: rejected for " << rejected_without << " readings with no floor on the\n"
            << "  spread, and for " << rejected_with << " with a floor of 0.001.\n\n";

  RC_CHECK_EQ(held_back, 4);
  RC_CHECK(rejected_without > 0);
  RC_CHECK_EQ(rejected_with, 0);
}
