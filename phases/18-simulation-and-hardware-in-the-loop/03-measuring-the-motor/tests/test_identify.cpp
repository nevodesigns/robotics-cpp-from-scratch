#include <rc/test/rc_test.hpp>
#include <rc/sim/plant.hpp>

#include <cmath>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <memory>
#include <vector>

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

// The motor on the bench, with a gain and time constant the experimenter does not know.
const double kGain = 2.0;
const double kTau = 0.1;
const double kLimit = 12.0;

struct Trace {
  std::vector<double> t;
  std::vector<double> v;
};

// A step of `command` from rest, read every 2 ms for 0.8 s, each speed measured with noise of the given standard deviation.
Trace step_response(double command, double noise, std::uint32_t seed) {
  rc::sim::SimMotor motor(kGain, kTau, kLimit);
  Rng rng{seed};
  Trace tr;
  for (int i = 1; i <= 400; ++i) {
    motor.apply(command, 0.002);
    tr.t.push_back(i * 0.002);
    tr.v.push_back(motor.velocity() + noise * rng.gaussian());
  }
  return tr;
}

// The other method: the time the speed first crosses 63.2 percent of its final value, which is one time constant.
double tau_from_crossing(const Trace& tr) {
  double last = 0.0;
  const std::size_t tail = tr.v.size() / 10;
  for (std::size_t i = tr.v.size() - tail; i < tr.v.size(); ++i) last += tr.v[i];
  last /= static_cast<double>(tail);
  for (std::size_t i = 0; i < tr.v.size(); ++i)
    if (tr.v[i] >= 0.632 * last) return tr.t[i];
  return 0.0;
}

}  // namespace

RC_TEST("the model's step response is zero at the start, 63 percent after one time constant and the final speed in the end") {
  RC_CHECK_NEAR(predict(2.0, 0.1, 3.0, 0.0), 0.0, 1e-12);
  RC_CHECK_NEAR(predict(2.0, 0.1, 3.0, 0.1), 6.0 * (1.0 - std::exp(-1.0)), 1e-12);
  RC_CHECK_NEAR(predict(2.0, 0.1, 3.0, 10.0), 6.0, 1e-4);
  RC_CHECK_NEAR(predict(2.0, 0.1, -3.0, 0.1), -6.0 * (1.0 - std::exp(-1.0)), 1e-12);
  RC_CHECK_NEAR(predict(2.0, 0.0, 3.0, 0.1), 0.0, 1e-12);          // no time constant: no model
  RC_CHECK_NEAR(rms_error({1.0, 2.0, 3.0}, {1.0, 2.0, 3.0}), 0.0, 1e-12);
  RC_CHECK_NEAR(rms_error({0.0, 0.0}, {3.0, 4.0}), std::sqrt(12.5), 1e-12);
  RC_CHECK_NEAR(rms_error({}, {1.0}), 0.0, 1e-12);
  RC_CHECK_NEAR(rms_error({1.0, 1.0, 1.0}, {0.0}), 1.0, 1e-12);    // only the points both have
}

RC_TEST("from a clean step the fit finds the gain and the time constant the motor was built with") {
  const Trace tr = step_response(3.0, 0.0, 1);
  const Fit f = fit_first_order(tr.t, tr.v, 3.0);
  std::cout << "  a clean step of 3 on a motor of gain 2 and time constant 0.1 s: found gain " << std::fixed << std::setprecision(4) << f.gain << ", time constant " << f.tau
            << " s, residual " << std::setprecision(5) << f.rms << "\n\n";
  RC_CHECK(f.ok);
  RC_CHECK_NEAR(f.gain, kGain, 0.01);
  RC_CHECK_NEAR(f.tau, kTau, 0.005);
  RC_CHECK(f.rms < 0.02);
}

RC_TEST("noise in the measurements spreads the estimate, more as the noise grows, and the fit says how far it is from the data") {
  std::cout << "  step of 3, 400 readings; 200 different noisy runs for each noise level\n  noise (speed units)   tau: mean   tau: spread   gain: spread   residual\n";
  double spread_small = 0.0;
  double spread_large = 0.0;
  double rms_small = 0.0;
  double rms_large = 0.0;
  for (double noise : {0.0, 0.05, 0.2, 0.5}) {
    double sum = 0.0;
    double sumsq = 0.0;
    double gsum = 0.0;
    double gsumsq = 0.0;
    double rsum = 0.0;
    int n = 0;
    for (int run = 0; run < 200; ++run) {
      const Trace tr = step_response(3.0, noise, 1000 + run);
      const Fit f = fit_first_order(tr.t, tr.v, 3.0);
      if (!f.ok) continue;
      sum += f.tau;
      sumsq += f.tau * f.tau;
      gsum += f.gain;
      gsumsq += f.gain * f.gain;
      rsum += f.rms;
      ++n;
    }
    const double mean = n ? sum / n : 0.0;
    const double spread = n ? std::sqrt(std::max(0.0, sumsq / n - mean * mean)) : 0.0;
    const double gmean = n ? gsum / n : 0.0;
    const double gspread = n ? std::sqrt(std::max(0.0, gsumsq / n - gmean * gmean)) : 0.0;
    std::cout << "  " << std::setw(19) << std::fixed << std::setprecision(2) << noise << "   " << std::setw(9) << std::setprecision(4) << mean << "   " << std::setw(11) << spread
              << "   " << std::setw(12) << gspread << "   " << std::setw(8) << std::setprecision(3) << (n ? rsum / n : 0.0) << "\n";
    if (noise == 0.05) {
      spread_small = spread;
      rms_small = n ? rsum / n : 0.0;
    }
    if (noise == 0.5) {
      spread_large = spread;
      rms_large = n ? rsum / n : 0.0;
    }
  }
  std::cout << "\n";
  RC_CHECK(spread_small > 0.0);
  RC_CHECK(spread_large > 3.0 * spread_small);
  RC_CHECK(rms_large > 3.0 * rms_small);
}

RC_TEST("the quick fit leans the wrong way when the readings are noisy, and searching for the smallest error removes the lean") {
  std::cout << "  200 noisy runs of a step of 3: the time constant found, the true one being 0.1000\n"
            << "  noise   quick fit: mean (spread)   refined: mean (spread)   read off a crossing: mean (spread)\n";
  double quick_mean = 0.0;
  double refined_mean = 0.0;
  double refined_spread = 0.0;
  double quick_spread = 0.0;
  for (double noise : {0.05, 0.3, 0.5}) {
    double q = 0.0, qq = 0.0, r = 0.0, rr = 0.0, c = 0.0, cc = 0.0;
    int n = 0;
    for (int run = 0; run < 200; ++run) {
      const Trace tr = step_response(3.0, noise, 5000 + run);
      const Fit quick = fit_first_order(tr.t, tr.v, 3.0);
      const Fit refined = refine_first_order(tr.t, tr.v, 3.0);
      if (!quick.ok || !refined.ok) continue;
      const double cross = tau_from_crossing(tr);
      q += quick.tau;
      qq += quick.tau * quick.tau;
      r += refined.tau;
      rr += refined.tau * refined.tau;
      c += cross;
      cc += cross * cross;
      ++n;
    }
    const double qm = n ? q / n : 0.0;
    const double rm = n ? r / n : 0.0;
    const double cm = n ? c / n : 0.0;
    const double qs = n ? std::sqrt(std::max(0.0, qq / n - qm * qm)) : 0.0;
    const double rs = n ? std::sqrt(std::max(0.0, rr / n - rm * rm)) : 0.0;
    const double cs = n ? std::sqrt(std::max(0.0, cc / n - cm * cm)) : 0.0;
    std::cout << "  " << std::setw(5) << std::fixed << std::setprecision(2) << noise << "   " << std::setw(8) << std::setprecision(4) << qm << " (" << qs << ")        " << std::setw(8) << rm
              << " (" << rs << ")           " << std::setw(8) << cm << " (" << cs << ")\n";
    if (noise == 0.5) {
      quick_mean = qm;
      refined_mean = rm;
      refined_spread = rs;
      quick_spread = qs;
    }
  }
  std::cout << "\n";
  RC_CHECK(quick_mean > 0.15);                                           // far from 0.1
  RC_CHECK(std::fabs(refined_mean - kTau) < 0.01);
  RC_CHECK(refined_spread < quick_spread + 0.02);
  RC_CHECK(refined_spread > 0.0);
}

RC_TEST("a model fitted on one step predicts another well inside the motor's range, and badly once the real motor is clamped") {
  const Trace small = step_response(1.0, 0.0, 1);                    // speed 2: well inside the limit
  const Fit f = fit_first_order(small.t, small.v, 1.0);
  RC_CHECK(f.ok);
  std::cout << "  fitted on a step of 1: gain " << std::fixed << std::setprecision(3) << f.gain << ", time constant " << std::setprecision(4) << f.tau << "\n"
            << "  predicting another step    command   rms error of the prediction (speed units)\n";
  double inside = 0.0;
  double clamped = 0.0;
  for (double command : {2.0, 4.0, 6.0, 12.0, 20.0}) {
    const Trace other = step_response(command, 0.0, 1);
    std::vector<double> model;
    for (double t : other.t) model.push_back(predict(f.gain, f.tau, command, t));
    const double e = rms_error(model, other.v);
    std::cout << "                             " << std::setw(7) << std::setprecision(1) << command << "   " << std::setw(10) << std::setprecision(4) << e << "\n";
    if (command == 6.0) inside = e;
    if (command == 20.0) clamped = e;
  }
  std::cout << "\n";
  RC_CHECK(inside < 0.05);
  RC_CHECK(clamped > 20.0 * inside);
}

RC_TEST("a trace that cannot be fitted is refused and not turned into numbers") {
  const Trace tr = step_response(3.0, 0.0, 1);
  RC_CHECK(fit_first_order(tr.t, tr.v, 3.0).ok);
  RC_CHECK(!fit_first_order(tr.t, tr.v, 0.0).ok);                    // no command: no step
  std::vector<double> few_t(tr.t.begin(), tr.t.begin() + 5);
  std::vector<double> few_v(tr.v.begin(), tr.v.begin() + 5);
  RC_CHECK(!fit_first_order(few_t, few_v, 3.0).ok);
  std::vector<double> shorter(tr.v.begin(), tr.v.end() - 1);
  RC_CHECK(!fit_first_order(tr.t, shorter, 3.0).ok);
  std::vector<double> flat(tr.t.size(), 0.0);
  RC_CHECK(!fit_first_order(tr.t, flat, 3.0).ok);                    // the motor never moved
  std::vector<double> too_fast(tr.t.size(), 5.0);
  RC_CHECK(!fit_first_order(tr.t, too_fast, 3.0).ok);                // already at speed from the first reading: no part of the rise was seen
}
