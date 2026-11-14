#include <rc/test/rc_test.hpp>
#include <sstream>
#include <cstring>

#include <cmath>
#include <iostream>

#include <rc/core/random.hpp>
#include <rc/field/sensors.hpp>

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
    {"wheel_deg", 3.103723942739561},
    {"gyro_deg", 68.01495919210885},
    {"fused_deg", 0.9604984308347958},
    {"naive_deg", 1.4477347120696258},
    {"bias_est", 0.00890113773874454},
    {"wheel_pos", 1.8168981615438562},
    {"gyro_pos", 14.119289403113308},
    {"fused_pos", 0.3228804570562675},
    {"naive_bad", 3.7411672726500753},
    {"fused_bad", 1.7117857515106862},
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

struct Run {
  double wheel_deg = 0, gyro_deg = 0, fused_deg = 0, naive_deg = 0;
  double wheel_pos = 0, gyro_pos = 0, fused_pos = 0;
  double bias_est = 0;
  int refused = 0;
};

// 120 s: 15 s straight then 10 s turning at 0.3 rad/s, alternately left and right; the wheels turn the body by 97 percent of what they say, and by half of it from 50 s to 52 s (a slip). The gyro has a bias of 0.01 rad/s, wandering, and noise.
Run simulate(bool slip, unsigned int seed, double eff_base = 0.97) {
  rc::field::ImuSpec spec;
  spec.gyro_bias = 0.01;
  spec.gyro_sigma = 0.005;
  spec.bias_walk = 1.0e-4;
  rc::field::Imu imu(spec, seed);
  rc::core::Rng noise(seed + 1000);
  FusedHeading fused(0.005, 0.05, 1.0e-4, 3.0);
  // A bias estimator that trusts the wheels equally whenever, for the comparison: a share of 0 and a gate that never refuses.
  FusedHeading naive(0.005, 0.0, 1.0e-4, 1.0e9);
  const double dt = 0.02;
  const double v = 0.5;
  double th_true = 0.0, th_wheel = 0.0, th_gyro = 0.0;
  double x_t = 0, y_t = 0, x_w = 0, y_w = 0, x_g = 0, y_g = 0, x_f = 0, y_f = 0;
  double sq_f = 0.0;
  int n = 0;
  for (int k = 0; k < 6000; ++k) {
    const double t = k * dt;
    const int cycle = static_cast<int>(t / 25.0);
    const double phase = t - 25.0 * cycle;
    const double w_cmd = phase < 15.0 ? 0.0 : ((cycle % 2 == 0) ? 0.3 : -0.3);
    const double eff = (slip && t >= 50.0 && t < 52.0) ? 0.5 : eff_base;
    const double w_true = eff * w_cmd;
    const double w_wheels = w_cmd + 0.01 * noise.gaussian();
    const double gyro = imu.measure(w_true, 0.0, 0.0, dt).gyro_z;
    th_true += w_true * dt;
    th_wheel += w_wheels * dt;
    th_gyro += gyro * dt;
    fused.update(gyro, w_wheels, dt);
    naive.update(gyro, w_wheels, dt);
    x_t += v * std::cos(th_true) * dt; y_t += v * std::sin(th_true) * dt;
    x_w += v * std::cos(th_wheel) * dt; y_w += v * std::sin(th_wheel) * dt;
    x_g += v * std::cos(th_gyro) * dt; y_g += v * std::sin(th_gyro) * dt;
    x_f += v * std::cos(fused.theta()) * dt; y_f += v * std::sin(fused.theta()) * dt;
    sq_f += (fused.theta() - th_true) * (fused.theta() - th_true);
    ++n;
  }
  Run r;
  r.wheel_deg = std::fabs(th_wheel - th_true) * 180.0 / kPi;
  r.gyro_deg = std::fabs(th_gyro - th_true) * 180.0 / kPi;
  r.fused_deg = std::fabs(fused.theta() - th_true) * 180.0 / kPi;
  r.naive_deg = std::fabs(naive.theta() - th_true) * 180.0 / kPi;
  r.wheel_pos = std::hypot(x_w - x_t, y_w - y_t);
  r.gyro_pos = std::hypot(x_g - x_t, y_g - y_t);
  r.fused_pos = std::hypot(x_f - x_t, y_f - y_t);
  r.bias_est = fused.filter().bias();
  r.refused = fused.filter().refused();
  (void)sq_f;
  (void)n;
  return r;
}

}  // namespace

RC_TEST("with no motion the bias of the gyro is read from the gyro, and with the wheels going straight it is read from the difference") {
  GyroBiasFilter f(0.005, 0.05, 1.0e-4, 3.0);
  for (int i = 0; i < 2500; ++i) f.update(0.01 + (i % 2 == 0 ? 0.003 : -0.003), 0.0, 0.02);
  RC_CHECK_NEAR(f.bias(), 0.01, 0.0015);
  RC_CHECK(f.variance() < 1e-5);
  // The correction it returns is the reading less the bias.
  RC_CHECK_NEAR(f.update(0.31, 0.3, 0.02), 0.31 - f.bias(), 1e-12);
}

RC_TEST("when the wheels are turning they are trusted less, and a difference far from the estimate is refused as a slip") {
  GyroBiasFilter straight(0.005, 0.05, 1.0e-4, 3.0);
  GyroBiasFilter turning(0.005, 0.05, 1.0e-4, 3.0);
  for (int i = 0; i < 100; ++i) {
    straight.update(0.05, 0.0, 0.02);   // a difference of 0.05 with the wheels straight
    turning.update(0.35, 0.3, 0.02);    // the same difference with the wheels turning at 0.3 rad/s
  }
  RC_CHECK(straight.bias() > turning.bias());
  RC_CHECK(straight.bias() > 0.03);
  GyroBiasFilter f(0.005, 0.05, 1.0e-4, 3.0);
  for (int i = 0; i < 2000; ++i) f.update(0.01, 0.0, 0.02);
  const double before = f.bias();
  f.update(0.2, 0.5, 0.02);  // the gyro says nothing like what the wheels say: a slip
  RC_CHECK(f.refused() >= 1);
  RC_CHECK_NEAR(f.bias(), before, 1e-3);
}

RC_TEST("over a run of two minutes the filter keeps the heading, where the gyro alone drifts and the wheels alone are caught by a slip") {
  const Run r = simulate(true, 3);
  rec("wheel_deg", r.wheel_deg, 1);
  rec("gyro_deg", r.gyro_deg, 1);
  rec("fused_deg", r.fused_deg, 1);
  rec("naive_deg", r.naive_deg, 1);
  rec("bias_est", r.bias_est, 4);
  rec("wheel_pos", r.wheel_pos, 1);
  rec("gyro_pos", r.gyro_pos, 1);
  rec("fused_pos", r.fused_pos, 1);
  std::cout << "  heading errors at the end: wheels " << r.wheel_deg << ", gyro " << r.gyro_deg << ", filter " << r.fused_deg << " (naive " << r.naive_deg << ") degrees; positions off by " << r.wheel_pos << ", " << r.gyro_pos << ", " << r.fused_pos << " m; bias " << r.bias_est << "\n";
  RC_CHECK(r.fused_deg < r.gyro_deg);
  RC_CHECK(r.fused_deg < r.wheel_deg);
  RC_CHECK(r.fused_pos < r.gyro_pos && r.fused_pos < r.wheel_pos);
  RC_CHECK_NEAR(r.bias_est, 0.01, 0.003);
  RC_CHECK(r.refused >= 1);
}

RC_TEST("weighing the wheels by how straight they are matters when the wheels are not well calibrated") {
  // The wheels' turn 12 percent too large (an effective track not calibrated): the naive estimate takes that for bias whenever the robot turns.
  double naive_sum = 0.0, fused_sum = 0.0;
  for (unsigned int seed = 1; seed <= 5; ++seed) {
    const Run r = simulate(false, seed, 0.88);
    naive_sum += r.naive_deg;
    fused_sum += r.fused_deg;
  }
  rec("naive_bad", naive_sum / 5.0, 1);
  rec("fused_bad", fused_sum / 5.0, 1);
  std::cout << "  wheels 12 percent out, mean of 5 runs: naive " << naive_sum / 5.0 << " degrees, weighed " << fused_sum / 5.0 << "\n";
  RC_CHECK(fused_sum < 0.7 * naive_sum);
}
