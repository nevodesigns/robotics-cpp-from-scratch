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
const double kSigma = 0.001;
const double kPi = 3.14159265358979323846;

// The rms of the filtered derivative of pure sensor noise on a joint standing still.
double noise_rms(double tf) {
  Rng rng{5};
  DerivativeFilter filter(tf);
  double sum = 0.0;
  int counted = 0;
  for (int i = 0; i < 5000; ++i) {
    const double d = filter.update(kSigma * rng.gaussian(), kDt);
    if (i < 500) continue;
    sum += d * d;
    ++counted;
  }
  return std::sqrt(sum / counted);
}

// The rms error of the filtered derivative of a noise free swing, half a radian at 1 Hz.
double lag_rms(double tf) {
  DerivativeFilter filter(tf);
  double sum = 0.0;
  int counted = 0;
  for (int i = 0; i < 5000; ++i) {
    const double t = i * kDt;
    const double d = filter.update(0.5 * std::sin(2.0 * kPi * t), kDt);
    if (i < 2000) continue;
    const double truth = 0.5 * 2.0 * kPi * std::cos(2.0 * kPi * (t - 0.5 * kDt));
    sum += (d - truth) * (d - truth);
    ++counted;
  }
  return std::sqrt(sum / counted);
}

// A position loop with a derivative term on a noisy sensor. The force is what the motor is asked for.
struct Chatter {
  double force_step_rms = 0.0;   // rms change in force from one step to the next
  double final_error = 0.0;
};

Chatter loop(double tf) {
  Rng rng{11};
  DerivativeFilter filter(tf);
  const double mass = 1.0;
  const double damping = 0.2;
  const double kp = 40.0;
  const double kd = 4.0;
  const double target = 1.0;
  double position = 0.0;
  double velocity = 0.0;
  double previous_force = 0.0;
  double sum = 0.0;
  int counted = 0;
  for (int i = 0; i < 4000; ++i) {
    const double z = position + kSigma * rng.gaussian();
    const double d = filter.update(z, kDt);
    const double force = kp * (target - z) - kd * d;
    velocity += (force - damping * velocity) / mass * kDt;
    position += velocity * kDt;
    if (i >= 2000) {
      sum += (force - previous_force) * (force - previous_force);
      ++counted;
    }
    previous_force = force;
  }
  Chatter c;
  c.force_step_rms = std::sqrt(sum / counted);
  c.final_error = target - position;
  return c;
}

}  // namespace

RC_TEST("the share of a new derivative let in is dt over tf plus dt, and everything for no filter") {
  RC_CHECK_NEAR(filter_alpha(0.02, 0.001), 0.001 / 0.021, 1e-12);
  RC_CHECK_NEAR(filter_alpha(0.0, 0.001), 1.0, 1e-12);
  RC_CHECK_NEAR(filter_alpha(-1.0, 0.001), 1.0, 1e-12);
  RC_CHECK_NEAR(filter_alpha(0.02, 0.0), 1.0, 1e-12);
}

RC_TEST("the first reading gives zero, and a steady slope comes through once the filter has settled") {
  DerivativeFilter filter(0.02);
  RC_CHECK_NEAR(filter.update(5.0, kDt), 0.0, 1e-12);
  double d = 0.0;
  double at_one_tf = 0.0;
  for (int i = 1; i <= 200; ++i) {
    d = filter.update(5.0 + 3.0 * i * kDt, kDt);
    if (i == 20) at_one_tf = d;
  }
  std::cout << "  a slope of 3 rad/s through a filter of 0.02 s: " << std::fixed << std::setprecision(3) << at_one_tf
            << " after one time constant, " << d << " after ten\n\n";
  RC_CHECK(at_one_tf > 1.7 && at_one_tf < 2.1);   // about 63 percent of 3
  RC_CHECK_NEAR(d, 3.0, 0.01);
}

RC_TEST("a filter on the derivative cuts the noise by a factor that grows with its time constant") {
  const double raw = noise_rms(0.0);
  std::cout << "  derivative of sensor noise, rms, sensor good to 0.001 rad at 1 kHz\n  tf (s)    rms (rad/s)   share of raw\n";
  double previous = raw;
  bool falls = true;
  for (double tf : {0.0, 0.005, 0.01, 0.02, 0.05}) {
    const double r = noise_rms(tf);
    std::cout << "  " << std::setw(6) << std::fixed << std::setprecision(3) << tf << "    " << std::setw(10)
              << std::setprecision(4) << r << "   " << std::setw(8) << std::setprecision(3) << r / raw << "\n";
    if (tf > 0.0 && !(r < previous)) falls = false;
    previous = r;
  }
  std::cout << "\n";
  RC_CHECK(raw > 0.5);
  RC_CHECK(falls);
  RC_CHECK(noise_rms(0.02) < 0.25 * raw);
}

RC_TEST("the filter lags a moving joint, and the lag grows with the time constant") {
  std::cout << "  derivative of a noise free swing, rms error\n  tf (s)    rms (rad/s)\n";
  double previous = -1.0;
  bool rises = true;
  for (double tf : {0.0, 0.005, 0.01, 0.02, 0.05}) {
    const double r = lag_rms(tf);
    std::cout << "  " << std::setw(6) << std::fixed << std::setprecision(3) << tf << "    " << std::setw(10)
              << std::setprecision(4) << r << "\n";
    if (tf > 0.0 && !(r > previous)) rises = false;
    previous = r;
  }
  std::cout << "\n";
  RC_CHECK(lag_rms(0.0) < 0.01);
  RC_CHECK(lag_rms(0.0) < lag_rms(0.005));
  RC_CHECK(rises);
}

RC_TEST("in a loop the motor chatters at the sensor's noise unless the derivative is filtered") {
  const Chatter raw = loop(0.0);
  const Chatter filtered = loop(0.02);
  std::cout << "  position loop, kp 40, kd 4, sensor good to 0.001 rad: change in force step to step, rms\n"
            << "    no filter  " << std::fixed << std::setprecision(3) << raw.force_step_rms << " N, error at the end "
            << std::setprecision(4) << raw.final_error << " rad\n"
            << "    tf 0.02 s  " << std::setprecision(3) << filtered.force_step_rms << " N, error at the end "
            << std::setprecision(4) << filtered.final_error << " rad\n\n";
  RC_CHECK(raw.force_step_rms > 1.0);
  RC_CHECK(filtered.force_step_rms < 0.25 * raw.force_step_rms);
  RC_CHECK(std::fabs(filtered.final_error) < 0.01);
}
