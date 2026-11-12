#include <rc/test/rc_test.hpp>

#include <cmath>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <limits>

#include "solution.hpp"

namespace {

// Deterministic noise: a small generator and Box-Muller, so the numbers are the same on every platform.
struct Noise {
  std::uint64_t s;
  explicit Noise(std::uint64_t seed) : s(seed) {}
  double uniform() {
    s = s * 6364136223846793005ULL + 1442695040888963407ULL;
    return (static_cast<double>(s >> 11) + 0.5) / 9007199254740992.0;
  }
  double gauss(double sigma) { return sigma * std::sqrt(-2.0 * std::log(uniform())) * std::cos(6.283185307179586 * uniform()); }
};

const double kNaN = std::numeric_limits<double>::quiet_NaN();

CrossConfig cfg(double tol, int debounce) {
  CrossConfig c;
  c.tolerance = tol;
  c.debounce = debounce;
  return c;
}

// How many times a healthy pair (noise 0.02 each, true value 1.0) trips the check in 100000 samples, with a reset after each trip.
int false_trips(double tol, int debounce) {
  CrossCheck ck(cfg(tol, debounce));
  Noise n(12345);
  int trips = 0;
  for (int i = 0; i < 100000; ++i) {
    if (ck.update(1.0 + n.gauss(0.02), 1.0 + n.gauss(0.02)).state == CheckState::faulted) {
      ++trips;
      ck.reset();
    }
  }
  return trips;
}

// How many samples after sensor B starts to drift away at 0.005 per sample the check latches (median of 25 runs with different noise).
int detect_samples(double tol, int debounce) {
  int results[25];
  for (int run = 0; run < 25; ++run) {
    CrossCheck ck(cfg(tol, debounce));
    Noise n(1000 + static_cast<std::uint64_t>(run));
    int found = 2000;
    for (int i = 0; i < 2000; ++i) {
      const double drift = 0.005 * i;
      if (ck.update(1.0 + n.gauss(0.02), 1.0 + drift + n.gauss(0.02)).state == CheckState::faulted) {
        found = i;
        break;
      }
    }
    results[run] = found;
  }
  for (int i = 0; i < 25; ++i)
    for (int j = i + 1; j < 25; ++j)
      if (results[j] < results[i]) std::swap(results[i], results[j]);
  return results[12];
}

}  // namespace

RC_TEST("two channels that agree give their mean, and the first disagreement is only a suspicion") {
  CrossCheck ck(cfg(0.1, 3));
  Verdict v = ck.update(1.00, 1.04);
  RC_CHECK(v.state == CheckState::ok);
  RC_CHECK_NEAR(v.value, 1.02, 1e-12);
  v = ck.update(1.00, 1.30);
  RC_CHECK(v.state == CheckState::suspect);
  RC_CHECK(v.reason == CheckReason::disagree);
  RC_CHECK_NEAR(v.value, 1.02, 1e-12);                 // the last agreed value is held
  v = ck.update(1.01, 1.03);
  RC_CHECK(v.state == CheckState::ok);                 // one bad sample is forgotten
  RC_CHECK_NEAR(v.value, 1.02, 1e-12);
  RC_CHECK(!ck.faulted());
}

RC_TEST("a fault needs the whole debounce in a row, latches, and says there is nothing to believe") {
  CrossCheck ck(cfg(0.1, 3));
  ck.update(1.0, 1.0);
  RC_CHECK(ck.update(1.0, 1.5).state == CheckState::suspect);
  RC_CHECK(ck.update(1.0, 1.5).state == CheckState::suspect);
  const Verdict v = ck.update(1.0, 1.5);
  RC_CHECK(v.state == CheckState::faulted);
  RC_CHECK(v.reason == CheckReason::disagree);
  RC_CHECK(std::isnan(v.value));
  RC_CHECK(ck.faulted());
  const Verdict after = ck.update(1.0, 1.0);           // they agree again: still faulted
  RC_CHECK(after.state == CheckState::faulted);
  RC_CHECK(std::isnan(after.value));
  ck.reset();
  RC_CHECK(ck.update(1.0, 1.0).state == CheckState::ok);
  std::cout << "  three bad samples in a row latched a fault; the two agreeing again did not clear it; reset did\n\n";
}

RC_TEST("a reading that is not a number or is out of range is a bad sample, and the reason names the channel") {
  CrossCheck ck(cfg(0.1, 2));
  ck.update(1.0, 1.0);
  Verdict v = ck.update(kNaN, 1.0);
  RC_CHECK(v.state == CheckState::suspect && v.reason == CheckReason::invalid_a);
  v = ck.update(kNaN, 1.0);
  RC_CHECK(v.state == CheckState::faulted && v.reason == CheckReason::invalid_a);
  ck.reset();
  ck.update(1.0, 1.0);
  v = ck.update(1.0, 250.0);
  RC_CHECK(v.reason == CheckReason::invalid_b);
  v = ck.update(1.0, -3.0);
  RC_CHECK(v.state == CheckState::faulted && v.reason == CheckReason::invalid_b);
  ck.reset();
  v = ck.update(kNaN, kNaN);
  RC_CHECK(v.reason == CheckReason::invalid_a);        // both bad: A is named first
}

RC_TEST("a tight tolerance and no debounce trip on noise, and a debounce and a wider tolerance cost detection time") {
  std::cout << "  noise 0.02 on each channel (the difference has sigma 0.028), 100000 healthy samples\n";
  std::cout << "  tolerance   debounce   false trips\n";
  const double tols[] = {0.04, 0.08, 0.12};
  int trips[3][2];
  for (int i = 0; i < 3; ++i) {
    trips[i][0] = false_trips(tols[i], 1);
    trips[i][1] = false_trips(tols[i], 3);
    std::cout << "  " << std::fixed << std::setprecision(2) << tols[i] << "        1          " << trips[i][0] << "\n  " << tols[i] << "        3          " << trips[i][1] << "\n";
  }
  std::cout << "\n  sensor B drifts away at 0.005 per sample: samples until the fault latches (median of 25 runs)\n  tolerance   debounce   samples\n";
  int det[3][2];
  for (int i = 0; i < 3; ++i) {
    det[i][0] = detect_samples(tols[i], 1);
    det[i][1] = detect_samples(tols[i], 3);
    std::cout << "  " << std::fixed << std::setprecision(2) << tols[i] << "        1          " << det[i][0] << "\n  " << tols[i] << "        3          " << det[i][1] << "\n";
  }
  std::cout << "\n";
  RC_CHECK(trips[0][0] > 10000);                       // 1.4 sigma and no debounce: constant false trips
  RC_CHECK(trips[0][1] < trips[0][0] / 10);            // a debounce of 3 cuts them by a factor of ten or more
  RC_CHECK(trips[1][0] > trips[1][1]);
  RC_CHECK(trips[2][1] == 0);                          // 4.2 sigma and a debounce of 3: none in 100000
  RC_CHECK(trips[2][0] < trips[1][0]);
  RC_CHECK(det[0][0] < det[1][0]);                     // a wider tolerance takes longer to see a drift
  RC_CHECK(det[1][0] < det[2][0]);
  RC_CHECK(det[1][1] > det[1][0]);                     // and so does a debounce
  RC_CHECK(det[2][1] > det[2][0]);
}

RC_TEST("two channels that are wrong in the same way agree, and the check does not see it") {
  CrossCheck ck(cfg(0.1, 3));
  // The truth is 3.0 m. Both sensors share a reference and both read 1.5: a common cause.
  Verdict v;
  for (int i = 0; i < 50; ++i) v = ck.update(1.5, 1.5);
  std::cout << "  truth 3.0 m, both channels read 1.5 m for 50 samples: state " << (v.state == CheckState::ok ? "ok" : "not ok") << ", value " << v.value << " m\n\n";
  RC_CHECK(v.state == CheckState::ok);
  RC_CHECK_NEAR(v.value, 1.5, 1e-12);
}
