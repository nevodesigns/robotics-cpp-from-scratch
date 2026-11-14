#include <rc/test/rc_test.hpp>
#include <sstream>
#include <cstring>

#include <cmath>
#include <iostream>

#include <rc/core/random.hpp>

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
    {"naive_bias", 0.48608439333076614},
    {"naive_rms", 0.49326212476672704},
    {"extrap_rms", 0.035225867907049714},
    {"history_rms", 0.04487825183799151},
    {"stop_extrap_err", 0.25},
    {"stop_history_err", 0.0},
    {"lag_found", 0.12},
    {"offset_deg", 2.291831180523293},
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


namespace {

const double kPi = 3.14159265358979323846;

struct Rms {
  double naive = 0.0, extrap = 0.0, history = 0.0;
  double naive_mean = 0.0;
};

// A robot driving along x at 2 m/s with accelerations (it speeds up and slows down), 60 s. An estimator of its position runs on an odometry that reads 1 percent long and is corrected once a second by a GNSS position that arrives 0.25 s late and good to 3 cm. Three ways of using the late fix.
Rms run(double delay) {
  rc::core::Rng rng(5);
  const double dt = 0.02;
  double x_true = 0.0, v_true = 2.0;
  double est_naive = 0.0, est_extrap = 0.0, est_hist = 0.0;
  StateHistory hist_h;
  struct Pending {
    double arrives;
    double stamp;
    double z;
    double v_at_stamp;
  };
  std::vector<Pending> pending;
  std::vector<double> truth_log;
  Rms r;
  int n = 0;
  double hist_x = 0.0;
  for (int k = 1; k <= 3000; ++k) {
    const double t = k * dt;
    v_true = 2.0 + 1.2 * std::sin(2.0 * kPi * t / 3.0);
    x_true += v_true * dt;
    const double v_odo = 1.01 * v_true;
    est_naive += v_odo * dt;
    est_extrap += v_odo * dt;
    hist_x += v_odo * dt;
    Stamped s;
    s.t = t;
    s.x = hist_x;
    hist_h.add(s);
    truth_log.push_back(x_true);
    if (k % 50 == 0) {
      Pending p;
      p.stamp = t;
      p.arrives = t + delay;
      p.z = x_true + 0.03 * rng.gaussian();
      p.v_at_stamp = 1.01 * v_true;  // what the estimator knows of the velocity: the odometry's
      pending.push_back(p);
    }
    for (std::size_t i = 0; i < pending.size();) {
      if (pending[i].arrives <= t + 1e-9) {
        const Pending p = pending[i];
        // naive: the fix is taken for the position now
        est_naive += 0.5 * (p.z - est_naive);
        // extrapolated: moved on by the velocity over the delay
        est_extrap += 0.5 * ((p.z + p.v_at_stamp * delay) - est_extrap);
        // history: compared with the state of its own time, and the correction carried to now
        const Correction c = delayed_correction(hist_h, p.stamp, p.z, 0.0, 0.5);
        if (c.valid) {
          est_hist += 0.0;  // est_hist follows hist_x below
          hist_x += c.dx;
        }
        pending.erase(pending.begin() + static_cast<long>(i));
      } else {
        ++i;
      }
    }
    est_hist = hist_x;
    if (k > 500) {
      r.naive += (est_naive - x_true) * (est_naive - x_true);
      r.extrap += (est_extrap - x_true) * (est_extrap - x_true);
      r.history += (est_hist - x_true) * (est_hist - x_true);
      r.naive_mean += est_naive - x_true;
      ++n;
    }
  }
  r.naive = std::sqrt(r.naive / n);
  r.extrap = std::sqrt(r.extrap / n);
  r.history = std::sqrt(r.history / n);
  r.naive_mean /= n;
  return r;
}

}  // namespace

RC_TEST("the state at a past time is found between the states either side of it, and not at all outside the history") {
  StateHistory h;
  for (int i = 0; i <= 10; ++i) {
    Stamped s;
    s.t = 0.1 * i;
    s.x = 2.0 * s.t;
    s.y = 1.0;
    s.theta = 0.5 * s.t;
    h.add(s);
  }
  Stamped out;
  RC_CHECK(h.at(0.45, out));
  RC_CHECK_NEAR(out.x, 0.9, 1e-12);
  RC_CHECK_NEAR(out.theta, 0.225, 1e-12);
  RC_CHECK(h.at(0.0, out) && h.at(1.0, out));
  RC_CHECK(!h.at(-0.01, out));
  RC_CHECK(!h.at(1.01, out));
  RC_CHECK(!StateHistory().at(0.0, out));
  RC_CHECK(h.size() == 11);
}

RC_TEST("a fix is compared with where the robot was then, and the correction is carried to now") {
  StateHistory h;
  for (int i = 0; i <= 100; ++i) {
    Stamped s;
    s.t = 0.01 * i;
    s.x = 2.0 * s.t;  // going at 2 m/s
    h.add(s);
  }
  // A fix of the position at t = 0.5 s (x = 1.0 truly) that says 1.1.
  const Correction c = delayed_correction(h, 0.5, 1.1, 0.0, 1.0);
  RC_CHECK(c.valid);
  RC_CHECK_NEAR(c.dx, 0.1, 1e-12);
  // Compared with the state now (x = 2.0) it would have been 0.9 out.
  Stamped now;
  h.at(1.0, now);
  RC_CHECK_NEAR(1.1 - now.x, -0.9, 1e-12);
  RC_CHECK(!delayed_correction(h, 5.0, 0.0, 0.0, 1.0).valid);
}

RC_TEST("a late fix taken for the present leaves a steady error of speed times delay; carrying the correction forward removes it") {
  const Rms r = run(0.25);
  rec("naive_bias", std::fabs(r.naive_mean), 2);
  rec("naive_rms", r.naive, 3);
  rec("extrap_rms", r.extrap, 3);
  rec("history_rms", r.history, 3);
  std::cout << "  rms error: " << r.naive << " m taking the fix for the present, " << r.extrap << " m extrapolating it, " << r.history << " m comparing it with the state of its own time\n";
  RC_CHECK(std::fabs(r.naive_mean) > 0.3);
  // Both ways of allowing for the delay are an order of magnitude better than none, and the same to within the noise of the fix.
  RC_CHECK(r.history < 0.2 * r.naive && r.extrap < 0.2 * r.naive);
  RC_CHECK(r.history < 2.0 * r.extrap);
}

RC_TEST("when the robot stops during the delay, moving the fix on by the velocity puts it a quarter of a metre beyond the robot, and the history does not") {
  // Going at 2 m/s until t = 10 s, then braking at 8 m/s2 to a stop at 10.25 s, 0.25 m further on. A fix stamped 10.0 s arrives at 10.25 s.
  StateHistory h;
  double x = 0.0;
  for (int k = 0; k <= 1100; ++k) {
    const double t = 0.01 * k;
    if (t <= 10.0) x = 2.0 * t;
    else if (t <= 10.25) x = 20.0 + 2.0 * (t - 10.0) - 4.0 * (t - 10.0) * (t - 10.0);
    else x = 20.25;
    Stamped s;
    s.t = t;
    s.x = x;
    h.add(s);
  }
  const double truth_now = 20.25;
  const double fix = 20.0;  // the position at the stamp, exactly
  const double extrapolated = fix + 2.0 * 0.25;
  Stamped then;
  RC_CHECK(h.at(10.0, then));
  const Correction c = delayed_correction(h, 10.0, fix, 0.0, 1.0);
  const double corrected = 20.25 + c.dx;  // the estimator now says 20.25; the fix agrees with its own history
  rec("stop_extrap_err", std::fabs(extrapolated - truth_now), 2);
  rec("stop_history_err", std::fabs(corrected - truth_now), 3);
  RC_CHECK_NEAR(std::fabs(extrapolated - truth_now), 0.25, 1e-9);
  RC_CHECK(std::fabs(corrected - truth_now) < 0.005);
}

RC_TEST("the delay between two sensors is found as the shift that makes them most alike") {
  rc::core::Rng rng(9);
  const double dt = 0.01;
  const int lag_samples = 12;  // 0.12 s
  std::vector<double> wheels, gyro;
  for (int i = 0; i < 2000; ++i) {
    const double t = i * dt;
    wheels.push_back(0.8 * std::sin(1.3 * t) + 0.5 * std::sin(0.41 * t + 1.0) + 0.05 * rng.gaussian());
  }
  for (int i = 0; i < 2000; ++i) {
    const int j = i - lag_samples;  // the gyro shows what the wheels did, 12 samples later
    gyro.push_back((j >= 0 ? wheels[static_cast<std::size_t>(j)] : 0.0) + 0.05 * rng.gaussian());
  }
  const int found = estimate_lag(wheels, gyro, 50);
  rec("lag_found", found * dt, 2);
  RC_CHECK(found == lag_samples);
  // A clock offset of 40 ms, at a turn rate of 1 rad/s, is an angle.
  rec("offset_deg", 1.0 * 0.04 * 180.0 / kPi, 1);
  RC_CHECK_NEAR(1.0 * 0.04 * 180.0 / kPi, 2.29, 0.01);
  RC_CHECK(estimate_lag(std::vector<double>(), gyro, 10) == 0);
}
