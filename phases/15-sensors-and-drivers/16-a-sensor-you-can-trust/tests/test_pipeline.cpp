#include <rc/test/rc_test.hpp>

#include <cmath>
#include <iomanip>
#include <iostream>

#include "solution.hpp"

namespace {
const double kDt = 0.01;   // a 100 Hz sensor

PipelineConfig base() { return PipelineConfig(); }
}  // namespace

RC_TEST("the stages run in order: calibrated first, so a steady raw reading comes out as scale times it plus offset") {
  PipelineConfig config = base();
  config.calibration.scale = 2.0;
  config.calibration.offset = 1.0;
  Pipeline pipeline(config);
  double y = 0.0;
  for (int i = 0; i < 400; ++i) y = pipeline.update(3.0);
  std::cout << "  raw 3.0 through a scale of 2 and an offset of 1 comes out as " << std::fixed << std::setprecision(4) << y << "\n\n";
  RC_CHECK_NEAR(y, 7.0, 1e-6);
}

RC_TEST("what is left of the noise is close to what the smoother predicts, the square root of alpha over 2 minus alpha") {
  std::cout << "  alpha   noise left   predicted\n";
  bool close = true;
  double first = 0.0;
  double last = 0.0;
  const double alphas[] = {1.0, 0.5, 0.2, 0.1, 0.05};
  for (double alpha : alphas) {
    PipelineConfig config = base();
    config.alpha = alpha;
    const double ratio = measure_noise_ratio(config);
    const double predicted = std::sqrt(alpha / (2.0 - alpha));
    std::cout << "  " << std::setw(5) << std::fixed << std::setprecision(2) << alpha << "   " << std::setw(10) << std::setprecision(3) << ratio
              << "   " << std::setw(9) << predicted << "\n";
    if (std::fabs(ratio - predicted) > 0.1 * predicted + 0.02) close = false;
    if (alpha == alphas[0]) first = ratio;
    last = ratio;
  }
  std::cout << "\n";
  RC_CHECK(first > 0.5);
  RC_CHECK(close);
  RC_CHECK(last < 0.4 * first);
}

RC_TEST("the output runs behind a ramp by the smoother's delay, (1 minus alpha) over alpha samples") {
  std::cout << "  alpha   lag (s)   predicted (s)\n";
  bool close = true;
  double slow = 0.0;
  double fast = 0.0;
  for (double alpha : {1.0, 0.5, 0.2, 0.1, 0.05}) {
    PipelineConfig config = base();
    config.alpha = alpha;
    const double lag = measure_ramp_lag(config, kDt);
    const double predicted = (1.0 - alpha) / alpha * kDt;
    std::cout << "  " << std::setw(5) << std::fixed << std::setprecision(2) << alpha << "   " << std::setw(7) << std::setprecision(4) << lag
              << "   " << std::setw(13) << predicted << "\n";
    if (std::fabs(lag - predicted) > 0.02 * predicted + 0.002) close = false;
    if (alpha == 1.0) fast = lag;
    if (alpha == 0.05) slow = lag;
  }
  std::cout << "\n";
  RC_CHECK(close);
  RC_CHECK(slow > fast + 0.1);
}

RC_TEST("rejecting wild readings first keeps a single spike from leaking through the smoother") {
  PipelineConfig with = base();
  PipelineConfig without = base();
  without.use_hampel = false;
  const double leak_with = measure_leak(with, 50.0);
  const double leak_without = measure_leak(without, 50.0);
  std::cout << "  one reading in a hundred is 50 against a true zero, smoother alpha 0.2: the output peaks at "
            << std::fixed << std::setprecision(3) << leak_with << " of the spike with the rejection filter, "
            << leak_without << " without it\n\n";
  RC_CHECK(leak_without > 0.15);
  RC_CHECK(leak_with < 0.02);
}

RC_TEST("the rejection filter holds a real step back, and the smoother adds to it: time to half way, by the settings") {
  std::cout << "  window   alpha   to half way (s)   without the rejection filter (s)\n";
  double w3 = 0.0;
  double w15 = 0.0;
  bool never_faster = true;
  for (std::size_t window : {3, 7, 15}) {
    for (double alpha : {0.5, 0.2}) {
      PipelineConfig config = base();
      config.window = window;
      config.alpha = alpha;
      PipelineConfig plain = config;
      plain.use_hampel = false;
      const double t = measure_step_time(config, kDt);
      const double p = measure_step_time(plain, kDt);
      std::cout << "  " << std::setw(6) << window << "   " << std::setw(5) << std::fixed << std::setprecision(2) << alpha << "   "
                << std::setw(15) << std::setprecision(2) << t << "   " << std::setw(30) << p << "\n";
      if (t < p) never_faster = false;
      if (window == 3 && alpha == 0.2) w3 = t;
      if (window == 15 && alpha == 0.2) w15 = t;
    }
  }
  std::cout << "\n";
  RC_CHECK(w3 >= 0.0);
  RC_CHECK(never_faster);
  RC_CHECK(w15 > w3);
}

RC_TEST("a quieter output lags more, and the table of the four costs is what to choose from") {
  std::cout << "  alpha   noise left   ramp lag (s)   step to half (s)   spike leak\n";
  double quiet_noise = 0.0;
  double quiet_lag = 0.0;
  double quick_noise = 0.0;
  double quick_lag = 0.0;
  for (double alpha : {1.0, 0.5, 0.2, 0.05}) {
    PipelineConfig config = base();
    config.alpha = alpha;
    const double n = measure_noise_ratio(config);
    const double lag = measure_ramp_lag(config, kDt);
    const double step = measure_step_time(config, kDt);
    const double leak = measure_leak(config, 50.0);
    std::cout << "  " << std::setw(5) << std::fixed << std::setprecision(2) << alpha << "   " << std::setw(10) << std::setprecision(3) << n << "   "
              << std::setw(12) << std::setprecision(4) << lag << "   " << std::setw(16) << std::setprecision(2) << step << "   " << std::setw(10)
              << std::setprecision(3) << leak << "\n";
    if (alpha == 0.05) {
      quiet_noise = n;
      quiet_lag = lag;
    }
    if (alpha == 1.0) {
      quick_noise = n;
      quick_lag = lag;
    }
  }
  std::cout << "\n";
  RC_CHECK(quick_noise > 0.0);
  RC_CHECK(quiet_noise < quick_noise);
  RC_CHECK(quiet_lag > quick_lag);
}
