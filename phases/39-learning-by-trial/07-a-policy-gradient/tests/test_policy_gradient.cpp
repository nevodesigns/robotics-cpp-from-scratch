#include <rc/test/rc_test.hpp>
#include <sstream>
#include <cstring>

#include <cmath>

#include "solution.hpp"
#include <iomanip>
#include <iostream>

// The numbers this lesson publishes: its page, its header and its quiz quote them, so each is held here as it was
// measured. A change that moves one by more than half a unit of its last printed digit fails the suite.
struct Pin {
  const char* key;
  double value;
};
static const Pin kPins[] = {
    {"k_best", 0.916},
    {"r_best", -0.030221777119180844},
    {"r_zero", -3.2990330271046124},
    {"sigma", 0.3},
    {"lr", 0.002},
    {"batch", 10.0},
    {"iters", 300.0},
    {"k_plain", 0.8982292258250701},
    {"r_plain", -0.030348348799836136},
    {"k_base", 0.8367274711593164},
    {"r_base", -0.032764934894892975},
    {"early", 50.0},
    {"sd_plain", 29.197740761672762},
    {"sd_base", 13.712157692957652},
};

static void rec(const char* key, double value, int digits = 3) {
  std::cout << "@" << key << " " << std::fixed << std::setprecision(digits) << value << std::endl;
  std::cout.unsetf(std::ios::floatfield);
  for (const Pin& pin : kPins) {
    if (std::strcmp(pin.key, key) != 0) continue;
    double allowed = 0.5;
    for (int i = 0; i < digits; ++i) allowed /= 10.0;
    if (!(std::fabs(value - pin.value) <= allowed)) {
      std::ostringstream message;
      message << "the published number " << key << " was " << pin.value << " and is now " << value;
      rc::test::record(__FILE__, __LINE__, message.str());
    }
  }
}
[[maybe_unused]] static void recs(const char* key, const std::string& value) { std::cout << "@" << key << " " << value << std::endl; }


static double spread(const std::vector<double>& v, std::size_t n) {
  double m = 0.0, q = 0.0;
  for (std::size_t i = 0; i < n; ++i) m += v[i] / n;
  for (std::size_t i = 0; i < n; ++i) q += (v[i] - m) * (v[i] - m) / n;
  return std::sqrt(q);
}

RC_TEST("the score of a run is the derivative of the log chance of its actions, and its mean over many runs is near zero") {
  const PgTask t;
  Rng r = make_rng(1);
  double s = 0.0, sum = 0.0;
  const int n = 20000;
  for (int i = 0; i < n; ++i) {
    pg_run(t, 0.5, 0.3, r, &s);
    sum += s;
  }
  // the expected score of any policy is zero, which is why a constant can be subtracted from G without bias
  RC_CHECK(std::fabs(sum / n) < 0.5);
  // the best gain by scanning lies between 0.5 and 1.2
  const double best = best_gain(t, 1.5, 1500, 2000, 1);
  RC_CHECK(best > 0.5 && best < 1.2);
}

RC_TEST("from a gain of zero the policy gradient climbs to near the best gain, with or without a baseline") {
  const PgTask t;
  const double k_best = best_gain(t, 1.5, 1500, 2000, 1);
  rec("k_best", k_best, 3);
  rec("r_best", mean_return(t, k_best, 2000, 1), 4);
  rec("r_zero", mean_return(t, 0.0, 2000, 1), 4);
  const double sigma = 0.3, lr = 0.002;
  const int batch = 10, iters = 300;
  const double kp = reinforce(t, sigma, lr, batch, iters, false, 3, nullptr);
  const double kb = reinforce(t, sigma, lr, batch, iters, true, 3, nullptr);
  rec("sigma", sigma, 1);
  rec("lr", lr, 3);
  rec("batch", batch, 0);
  rec("iters", iters, 0);
  rec("k_plain", kp, 3);
  rec("r_plain", mean_return(t, kp, 2000, 1), 4);
  rec("k_base", kb, 3);
  rec("r_base", mean_return(t, kb, 2000, 1), 4);
  RC_CHECK(std::fabs(kp - k_best) < 0.15 && std::fabs(kb - k_best) < 0.15);
  RC_CHECK(mean_return(t, kp, 2000, 1) > 0.9 * mean_return(t, k_best, 2000, 1) + 0.1 * mean_return(t, 0.0, 2000, 1));
}

RC_TEST("subtracting the batch's mean return makes the gradient estimates far less noisy") {
  const PgTask t;
  std::vector<double> ep, eb;
  reinforce(t, 0.3, 0.002, 10, 300, false, 3, &ep);
  reinforce(t, 0.3, 0.002, 10, 300, true, 3, &eb);
  const std::size_t early = 50;
  rec("early", static_cast<double>(early), 0);
  rec("sd_plain", spread(ep, early), 1);
  rec("sd_base", spread(eb, early), 1);
  RC_CHECK(spread(eb, early) < 0.7 * spread(ep, early));
}
