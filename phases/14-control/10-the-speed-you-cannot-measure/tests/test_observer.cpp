#include <rc/test/rc_test.hpp>

#include <cmath>
#include <cstdint>
#include <iomanip>
#include <iostream>

#include "solution.hpp"

namespace {

struct Rng {
  std::uint32_t state;
  std::uint32_t next() {
    state ^= state << 13;
    state ^= state >> 17;
    state ^= state << 5;
    return state;
  }
  double uniform() { return static_cast<double>(next() >> 8) / 16777216.0; }
  double gaussian() {
    double sum = 0.0;
    for (int i = 0; i < 12; ++i) sum += uniform();
    return sum - 6.0;
  }
};

const double kDt = 0.001;
const double kSigma = 0.001;          // a sensor good to a thousandth of a radian
const double kPi = 3.14159265358979323846;

// A joint swinging half a radian either side at one hertz: p, v and a are exact.
struct Swing {
  static double p(double t) { return 0.5 * std::sin(2.0 * kPi * t); }
  static double v(double t) { return 0.5 * 2.0 * kPi * std::cos(2.0 * kPi * t); }
  static double a(double t) { return -0.5 * 4.0 * kPi * kPi * std::sin(2.0 * kPi * t); }
};

struct Errors {
  double difference = 0.0;
  double observer = 0.0;
};

// The rms speed error over `seconds`, after one second to settle, of a plain difference of
// two readings and of an observer at `bandwidth`. `use_model` says whether the observer is
// told the acceleration.
Errors run(double bandwidth, bool use_model, double sigma, bool moving) {
  Rng rng{9};
  VelocityObserver observer(observer_gains(bandwidth));
  observer.reset(0.0, moving ? Swing::v(0.0) : 0.0);
  double previous = moving ? Swing::p(0.0) : 0.0;

  double sum_difference = 0.0;
  double sum_observer = 0.0;
  int counted = 0;
  const int steps = 5000;
  for (int i = 1; i <= steps; ++i) {
    const double t = i * kDt;
    const double truth_p = moving ? Swing::p(t) : 0.0;
    const double truth_v = moving ? Swing::v(t) : 0.0;
    const double truth_a = moving ? Swing::a(t) : 0.0;
    const double z = truth_p + sigma * rng.gaussian();
    observer.update(z, use_model ? truth_a : 0.0, kDt);
    const double difference = (z - previous) / kDt;
    previous = z;
    if (t < 1.0) continue;
    sum_difference += (difference - truth_v) * (difference - truth_v);
    sum_observer += (observer.velocity() - truth_v) * (observer.velocity() - truth_v);
    ++counted;
  }
  Errors e;
  e.difference = std::sqrt(sum_difference / counted);
  e.observer = std::sqrt(sum_observer / counted);
  return e;
}

}  // namespace

RC_TEST("both gains come from one number: 2 w and w squared, and none for no bandwidth") {
  const ObserverGains g = observer_gains(50.0);
  RC_CHECK_NEAR(g.position, 100.0, 1e-12);
  RC_CHECK_NEAR(g.velocity, 2500.0, 1e-12);
  const ObserverGains none = observer_gains(0.0);
  RC_CHECK_NEAR(none.position, 0.0, 1e-12);
  RC_CHECK_NEAR(none.velocity, 0.0, 1e-12);
  const ObserverGains negative = observer_gains(-5.0);
  RC_CHECK_NEAR(negative.velocity, 0.0, 1e-12);
}

RC_TEST("started with the speed wrong by 2 rad/s and no noise, the error dies at the pole") {
  VelocityObserver observer(observer_gains(50.0));
  observer.reset(0.0, 2.0);                      // truth is standing still at zero

  double error_at_tenth = 0.0;
  double error_at_second = 0.0;
  for (int i = 1; i <= 1000; ++i) {
    observer.update(0.0, 0.0, kDt);
    if (i == 100) error_at_tenth = std::fabs(observer.velocity());
    if (i == 1000) error_at_second = std::fabs(observer.velocity());
  }
  std::cout << "  speed error from a start 2 rad/s out, bandwidth 50: " << std::fixed << std::setprecision(6)
            << error_at_tenth << " after 0.1 s, " << error_at_second << " after 1 s\n\n";
  RC_CHECK(error_at_tenth < 0.3);
  RC_CHECK(error_at_second < 1e-6);
}

RC_TEST("a plain difference of two readings is a hundred times noisier than the observer") {
  const Errors e = run(50.0, true, kSigma, true);
  std::cout << "  speed error, rms, with a sensor good to 0.001 rad at 1 kHz: difference " << std::fixed
            << std::setprecision(3) << e.difference << " rad/s, observer " << e.observer << " rad/s\n\n";
  RC_CHECK(e.observer > 0.0);
  RC_CHECK(e.difference > 100.0 * e.observer);
}

RC_TEST("told the acceleration it follows the motion, and without it the estimate lags") {
  const Errors with = run(50.0, true, 0.0, true);
  const Errors without = run(50.0, false, 0.0, true);
  std::cout << "  noise free swing, bandwidth 50: with the model " << std::scientific << std::setprecision(2)
            << with.observer << " rad/s, ignoring it " << without.observer << " rad/s\n\n";
  RC_CHECK(with.observer > 0.0);
  RC_CHECK(without.observer > 10.0 * with.observer);
}

RC_TEST("a push the model does not know about leaves a speed error of 2 d over w") {
  // The joint accelerates at d = 1 rad/s^2 and the observer is told zero.
  const double d = 1.0;
  const double w = 50.0;
  Rng rng{1};
  (void)rng;
  VelocityObserver observer(observer_gains(w));
  observer.reset(0.0, 0.0);
  double error = 0.0;
  for (int i = 1; i <= 5000; ++i) {
    const double t = i * kDt;
    observer.update(0.5 * d * t * t, 0.0, kDt);
    error = d * t - observer.velocity();
  }
  const double predicted = 2.0 * d / w;
  std::cout << "  unmodelled acceleration of 1 rad/s^2, bandwidth 50: speed error " << std::fixed
            << std::setprecision(4) << error << " rad/s, predicted 2 d / w = " << predicted << "\n\n";
  RC_CHECK_NEAR(error, predicted, 0.05 * predicted);
}

RC_TEST("more bandwidth tracks faster and passes more noise, and the two cross") {
  const double bands[] = {5.0, 20.0, 50.0, 100.0, 200.0, 400.0};
  std::cout << "  bandwidth   noise only   tracking only   both\n";
  double previous_noise = 0.0;
  double best_total = 1e9;
  double best_band = 0.0;
  double first_total = 0.0;
  double last_total = 0.0;
  bool noise_rises = true;
  for (double w : bands) {
    const double noise = run(w, false, kSigma, false).observer;
    const double lag = run(w, false, 0.0, true).observer;
    const double both = run(w, false, kSigma, true).observer;
    std::cout << "  " << std::setw(9) << std::fixed << std::setprecision(0) << w << "   " << std::setprecision(4)
              << std::setw(10) << noise << "   " << std::setw(13) << lag << "   " << std::setw(6) << both << "\n";
    if (noise <= previous_noise) noise_rises = false;
    previous_noise = noise;
    if (both < best_total) {
      best_total = both;
      best_band = w;
    }
    if (w == bands[0]) first_total = both;
    last_total = both;
  }
  std::cout << "  the best of these is a bandwidth of " << std::setprecision(0) << best_band << "\n\n";
  RC_CHECK(noise_rises);
  RC_CHECK(best_total < first_total);
  RC_CHECK(best_total < last_total);
}
